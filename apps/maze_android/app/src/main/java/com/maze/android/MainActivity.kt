package com.maze.android

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.aspectRatio
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.Button
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.DisposableEffect
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.text.input.KeyboardType
import androidx.compose.ui.unit.dp
import com.maze.nativebridge.MazeNativeBridge
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.FlowPreview
import kotlinx.coroutines.Job
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.collect
import kotlinx.coroutines.flow.sample
import kotlinx.coroutines.flow.update
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import org.json.JSONObject

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContent {
            MaterialTheme {
                MazeAndroidScreen()
            }
        }
    }
}

private data class CellCoord(val row: Int, val col: Int)

private data class CellDelta(val row: Int, val col: Int, val state: Int)

private data class TopologySnapshot(
    val width: Int,
    val height: Int,
    val start: CellCoord,
    val end: CellCoord,
    val wallMasks: List<Int>
)

private data class MazeRenderState(
    val mazeWidth: Int = 0,
    val mazeHeight: Int = 0,
    val start: CellCoord = CellCoord(0, 0),
    val end: CellCoord = CellCoord(0, 0),
    val wallMasks: List<Int> = emptyList(),
    val cellStates: List<Int> = emptyList(),
    val pathCells: Set<CellCoord> = emptySet(),
    val hasTopologySnapshot: Boolean = false
) {
    fun consume(event: MazeEventEnvelope): MazeRenderState {
        return when (event.type) {
            MazeNativeBridge.EVENT_TOPOLOGY_SNAPSHOT -> {
                val topology = parseTopologySnapshot(event.payload) ?: return this
                copy(
                    mazeWidth = topology.width,
                    mazeHeight = topology.height,
                    start = topology.start,
                    end = topology.end,
                    wallMasks = topology.wallMasks,
                    cellStates = List(topology.width * topology.height) { 0 },
                    pathCells = emptySet(),
                    hasTopologySnapshot = true
                )
            }

            MazeNativeBridge.EVENT_CELL_STATE_CHANGED,
            MazeNativeBridge.EVENT_PROGRESS,
            MazeNativeBridge.EVENT_PATH_UPDATED -> {
                if (!hasTopologySnapshot || mazeWidth <= 0 || mazeHeight <= 0) {
                    return this
                }

                val updatedStates = cellStates.toMutableList()
                if (event.type == MazeNativeBridge.EVENT_CELL_STATE_CHANGED ||
                    event.type == MazeNativeBridge.EVENT_PROGRESS
                ) {
                    parseDeltas(event.payload).forEach { delta ->
                        val index = cellIndex(delta.row, delta.col, mazeWidth)
                        if (delta.row in 0 until mazeHeight &&
                            delta.col in 0 until mazeWidth &&
                            index in updatedStates.indices
                        ) {
                            updatedStates[index] = delta.state
                        }
                    }
                }

                val updatedPath =
                    if (event.type == MazeNativeBridge.EVENT_PROGRESS ||
                        event.type == MazeNativeBridge.EVENT_PATH_UPDATED
                    ) {
                        parsePath(event.payload).toSet()
                    } else {
                        pathCells
                    }

                copy(cellStates = updatedStates, pathCells = updatedPath)
            }

            else -> this
        }
    }
}

private data class MazeEventEnvelope(
    val seq: Long,
    val type: Int,
    val payload: String
)

private data class MazeEventSnapshot(
    val eventCount: Int = 0,
    val progressCount: Int = 0,
    val topologyCount: Int = 0,
    val lastSeq: Long = 0L,
    val lastType: Int = -1,
    val lastPayload: String = "",
    val lastMessage: String = "",
    val renderState: MazeRenderState = MazeRenderState()
) {
    fun consume(event: MazeEventEnvelope): MazeEventSnapshot {
        val message = parseMessage(event.payload)
        return copy(
            eventCount = eventCount + 1,
            progressCount = progressCount + if (event.type == MazeNativeBridge.EVENT_PROGRESS) 1 else 0,
            topologyCount = topologyCount + if (event.type == MazeNativeBridge.EVENT_TOPOLOGY_SNAPSHOT) 1 else 0,
            lastSeq = event.seq,
            lastType = event.type,
            lastPayload = event.payload,
            lastMessage = if (message.isNotEmpty()) message else lastMessage,
            renderState = renderState.consume(event)
        )
    }
}

private const val WALL_TOP = 1 shl 0
private const val WALL_RIGHT = 1 shl 1
private const val WALL_BOTTOM = 1 shl 2
private const val WALL_LEFT = 1 shl 3

private fun cellIndex(row: Int, col: Int, width: Int): Int = (row * width) + col

private fun parseTopologySnapshot(payload: String): TopologySnapshot? {
    return runCatching {
        val root = JSONObject(payload)
        val width = root.getInt("width")
        val height = root.getInt("height")
        val startJson = root.getJSONObject("start")
        val endJson = root.getJSONObject("end")
        val wallsJson = root.getJSONArray("walls")
        val wallMasks = buildList(wallsJson.length()) {
            for (index in 0 until wallsJson.length()) {
                add(wallsJson.getInt(index))
            }
        }
        TopologySnapshot(
            width = width,
            height = height,
            start = CellCoord(startJson.getInt("row"), startJson.getInt("col")),
            end = CellCoord(endJson.getInt("row"), endJson.getInt("col")),
            wallMasks = wallMasks
        )
    }.getOrNull()
}

private fun parseDeltas(payload: String): List<CellDelta> {
    return runCatching {
        val deltasJson = JSONObject(payload).optJSONArray("deltas") ?: return emptyList()
        buildList(deltasJson.length()) {
            for (index in 0 until deltasJson.length()) {
                val item = deltasJson.getJSONObject(index)
                add(CellDelta(item.getInt("row"), item.getInt("col"), item.getInt("state")))
            }
        }
    }.getOrElse { emptyList() }
}

private fun parsePath(payload: String): List<CellCoord> {
    return runCatching {
        val pathJson = JSONObject(payload).optJSONArray("path") ?: return emptyList()
        buildList(pathJson.length()) {
            for (index in 0 until pathJson.length()) {
                val item = pathJson.getJSONObject(index)
                add(CellCoord(item.getInt("row"), item.getInt("col")))
            }
        }
    }.getOrElse { emptyList() }
}

private fun parseMessage(payload: String): String {
    return runCatching { JSONObject(payload).optString("message", "") }.getOrDefault("")
}

@OptIn(FlowPreview::class)
@Composable
private fun MazeAndroidScreen() {
    var width by remember { mutableStateOf("8") }
    var height by remember { mutableStateOf("8") }
    var unitPixels by remember { mutableStateOf("20") }
    var generation by remember { mutableStateOf("DFS") }
    var search by remember { mutableStateOf("BFS") }
    var summary by remember { mutableStateOf("") }
    var status by remember { mutableStateOf("Ready") }
    var lastError by remember { mutableStateOf("") }
    var apiVersion by remember { mutableStateOf(MazeNativeBridge.nativeGetApiVersion()) }
    var running by remember { mutableStateOf(false) }
    var eventSnapshot by remember { mutableStateOf(MazeEventSnapshot()) }
    var runJob by remember { mutableStateOf<Job?>(null) }

    val eventState = remember { MutableStateFlow(MazeEventSnapshot()) }
    val scope = rememberCoroutineScope()
    val sessionPtr = remember { mutableStateOf(0L) }
    val listener = remember(eventState) {
        object : MazeNativeBridge.EventListener {
            override fun onMazeEvent(seq: Long, type: Int, payload: String) {
                eventState.update { it.consume(MazeEventEnvelope(seq, type, payload)) }
            }
        }
    }

    LaunchedEffect(eventState) {
        eventState.sample(50L).collect { sampled ->
            eventSnapshot = sampled
            status = when (sampled.lastType) {
                MazeNativeBridge.EVENT_RUN_STARTED -> "Run started"
                MazeNativeBridge.EVENT_PROGRESS -> "Running..."
                MazeNativeBridge.EVENT_RUN_FINISHED -> "Run finished"
                MazeNativeBridge.EVENT_RUN_CANCELLED -> "Run cancelled"
                MazeNativeBridge.EVENT_RUN_FAILED -> {
                    if (sampled.lastMessage.isNotEmpty()) {
                        "Run failed: ${sampled.lastMessage}"
                    } else {
                        "Run failed"
                    }
                }

                else -> status
            }
        }
    }

    DisposableEffect(listener) {
        sessionPtr.value = MazeNativeBridge.nativeCreateSession()
        if (sessionPtr.value == 0L) {
            status = "MazeSessionCreate failed"
        } else {
            val listenerCode = MazeNativeBridge.nativeSetEventListener(sessionPtr.value, listener)
            if (listenerCode != MazeNativeBridge.STATUS_OK) {
                status = "nativeSetEventListener failed, code=$listenerCode"
            }
        }
        onDispose {
            runJob?.cancel()
            if (sessionPtr.value != 0L) {
                MazeNativeBridge.nativeSetEventListener(sessionPtr.value, null)
                MazeNativeBridge.nativeCancel(sessionPtr.value)
                MazeNativeBridge.nativeDestroySession(sessionPtr.value)
                sessionPtr.value = 0L
            }
        }
    }

    Column(
        modifier = Modifier
            .fillMaxSize()
            .verticalScroll(rememberScrollState())
            .padding(16.dp),
        verticalArrangement = Arrangement.Top
    ) {
        Text("Maze Android (Phase 4/5)")
        Text("JNI API Version: $apiVersion")
        Spacer(modifier = Modifier.height(8.dp))

        Row(Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.spacedBy(8.dp)) {
            IntField("Width", width, modifier = Modifier.weight(1f)) { width = it }
            IntField("Height", height, modifier = Modifier.weight(1f)) { height = it }
        }
        Row(Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.spacedBy(8.dp)) {
            IntField("UnitPixels", unitPixels, modifier = Modifier.weight(1f)) { unitPixels = it }
            OutlinedTextField(
                modifier = Modifier.weight(1f),
                value = generation,
                onValueChange = { generation = it },
                label = { Text("Generation") }
            )
        }
        Row(Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.spacedBy(8.dp)) {
            OutlinedTextField(
                modifier = Modifier.weight(1f),
                value = search,
                onValueChange = { search = it },
                label = { Text("Search") }
            )
            Text(
                modifier = Modifier.weight(1f),
                text = "Realtime event-driven Canvas rendering"
            )
        }

        Spacer(modifier = Modifier.height(8.dp))
        MazeCanvas(
            renderState = eventSnapshot.renderState,
            modifier = Modifier.fillMaxWidth()
        )

        Spacer(modifier = Modifier.height(8.dp))
        Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
            Button(
                enabled = !running,
                onClick = {
                    if (sessionPtr.value == 0L) {
                        status = "Session not ready"
                        return@Button
                    }

                    val requestJson = """
                        {
                          "schema_version": 1,
                          "request": {
                            "maze": {
                              "width": ${width.toIntOrNull() ?: 0},
                              "height": ${height.toIntOrNull() ?: 0}
                            },
                            "algorithms": {
                              "generation": "${generation.trim()}",
                              "search": "${search.trim()}"
                            },
                            "visualization": {
                              "unit_pixels": ${unitPixels.toIntOrNull() ?: 0}
                            },
                            "stream": {
                              "emit_stride": 1,
                              "emit_progress": 1
                            }
                          }
                        }
                    """.trimIndent()

                    eventState.value = MazeEventSnapshot()
                    eventSnapshot = MazeEventSnapshot()
                    summary = ""
                    lastError = ""
                    apiVersion = MazeNativeBridge.nativeGetApiVersion()
                    val configureCode =
                        MazeNativeBridge.nativeConfigureSession(sessionPtr.value, requestJson)
                    if (configureCode != MazeNativeBridge.STATUS_OK) {
                        lastError = MazeNativeBridge.nativeGetLastError(sessionPtr.value)
                        status = "Configure failed, code=$configureCode"
                        return@Button
                    }

                    running = true
                    status = "Running..."
                    runJob = scope.launch {
                        val code = withContext(Dispatchers.IO) {
                            MazeNativeBridge.nativeRunSession(sessionPtr.value)
                        }
                        summary = MazeNativeBridge.nativeGetSummaryJson(sessionPtr.value)
                        lastError = MazeNativeBridge.nativeGetLastError(sessionPtr.value)
                        running = false
                        status = when (code) {
                            MazeNativeBridge.STATUS_OK -> "Run success"
                            MazeNativeBridge.STATUS_CANCELLED -> "Run cancelled"
                            else -> "Run failed, code=$code"
                        }
                    }
                }
            ) {
                Text("Run")
            }
            Button(
                enabled = running && sessionPtr.value != 0L,
                onClick = {
                    if (sessionPtr.value != 0L) {
                        val code = MazeNativeBridge.nativeCancel(sessionPtr.value)
                        status = "Cancel requested, code=$code"
                    }
                }
            ) {
                Text("Cancel")
            }
        }

        Spacer(modifier = Modifier.height(12.dp))
        Text("Status: $status")
        Text("Running: $running")
        Text("LastError: $lastError")
        Spacer(modifier = Modifier.height(6.dp))
        Text("EventCount: ${eventSnapshot.eventCount}")
        Text("ProgressEvents: ${eventSnapshot.progressCount}")
        Text("TopologyEvents: ${eventSnapshot.topologyCount}")
        Text("LastEvent: ${EventTypeLabel(eventSnapshot.lastType)}")
        Text("LastSeq: ${eventSnapshot.lastSeq}")
        Spacer(modifier = Modifier.height(6.dp))
        Text("LastPayload:")
        Text(eventSnapshot.lastPayload.ifEmpty { "(empty)" })
        Spacer(modifier = Modifier.height(6.dp))
        Text("Summary:")
        Text(summary.ifEmpty { "{}" })
    }
}

@Composable
private fun MazeCanvas(renderState: MazeRenderState, modifier: Modifier = Modifier) {
    Column(modifier = modifier) {
        if (!renderState.hasTopologySnapshot || renderState.mazeWidth <= 0 || renderState.mazeHeight <= 0) {
            Text("Canvas: waiting for topology snapshot...")
            return@Column
        }

        val aspectRatio = renderState.mazeWidth.toFloat() / renderState.mazeHeight.toFloat()
        Box(
            modifier = Modifier
                .fillMaxWidth()
                .aspectRatio(aspectRatio)
        ) {
            Canvas(modifier = Modifier.fillMaxSize()) {
                val cellWidth = size.width / renderState.mazeWidth
                val cellHeight = size.height / renderState.mazeHeight
                val wallStroke = (minOf(cellWidth, cellHeight) * 0.1f).coerceAtLeast(2f)

                fun wallMaskAt(row: Int, col: Int): Int {
                    val index = cellIndex(row, col, renderState.mazeWidth)
                    return renderState.wallMasks.getOrElse(index) {
                        WALL_TOP or WALL_RIGHT or WALL_BOTTOM or WALL_LEFT
                    }
                }

                fun cellStateAt(row: Int, col: Int): Int {
                    val index = cellIndex(row, col, renderState.mazeWidth)
                    return renderState.cellStates.getOrElse(index) { 0 }
                }

                fun cellColor(row: Int, col: Int): Color {
                    val coord = CellCoord(row, col)
                    return when {
                        coord == renderState.start -> Color(0xFF3498DB)
                        coord == renderState.end -> Color(0xFFE67E22)
                        coord in renderState.pathCells -> Color(0xFF43A0FF)
                        else -> when (cellStateAt(row, col)) {
                            3 -> Color(0xFFF6C45A)
                            4 -> Color(0xFF42A5F5)
                            5 -> Color(0xFFB0B0B0)
                            6 -> Color(0xFF4CAF50)
                            else -> Color(0xFFF5F5F5)
                        }
                    }
                }

                for (row in 0 until renderState.mazeHeight) {
                    for (col in 0 until renderState.mazeWidth) {
                        val left = col * cellWidth
                        val top = row * cellHeight
                        val right = left + cellWidth
                        val bottom = top + cellHeight

                        drawRect(
                            color = cellColor(row, col),
                            topLeft = Offset(left, top),
                            size = Size(cellWidth, cellHeight)
                        )

                        val mask = wallMaskAt(row, col)
                        val wallColor = Color(0xFF202020)
                        if ((mask and WALL_TOP) != 0) {
                            drawLine(
                                color = wallColor,
                                start = Offset(left, top),
                                end = Offset(right, top),
                                strokeWidth = wallStroke
                            )
                        }
                        if ((mask and WALL_RIGHT) != 0) {
                            drawLine(
                                color = wallColor,
                                start = Offset(right, top),
                                end = Offset(right, bottom),
                                strokeWidth = wallStroke
                            )
                        }
                        if ((mask and WALL_BOTTOM) != 0) {
                            drawLine(
                                color = wallColor,
                                start = Offset(left, bottom),
                                end = Offset(right, bottom),
                                strokeWidth = wallStroke
                            )
                        }
                        if ((mask and WALL_LEFT) != 0) {
                            drawLine(
                                color = wallColor,
                                start = Offset(left, top),
                                end = Offset(left, bottom),
                                strokeWidth = wallStroke
                            )
                        }
                    }
                }

                drawRect(color = Color(0xFF101010), style = Stroke(width = wallStroke))
            }
        }
    }
}

private fun EventTypeLabel(eventType: Int): String {
    return when (eventType) {
        MazeNativeBridge.EVENT_RUN_STARTED -> "RUN_STARTED"
        MazeNativeBridge.EVENT_CELL_STATE_CHANGED -> "CELL_STATE_CHANGED"
        MazeNativeBridge.EVENT_PATH_UPDATED -> "PATH_UPDATED"
        MazeNativeBridge.EVENT_PROGRESS -> "PROGRESS"
        MazeNativeBridge.EVENT_RUN_FINISHED -> "RUN_FINISHED"
        MazeNativeBridge.EVENT_RUN_CANCELLED -> "RUN_CANCELLED"
        MazeNativeBridge.EVENT_RUN_FAILED -> "RUN_FAILED"
        MazeNativeBridge.EVENT_TOPOLOGY_SNAPSHOT -> "TOPOLOGY_SNAPSHOT"
        else -> "NONE"
    }
}

@Composable
private fun IntField(
    label: String,
    value: String,
    modifier: Modifier = Modifier,
    onChange: (String) -> Unit
) {
    OutlinedTextField(
        modifier = modifier,
        value = value,
        onValueChange = onChange,
        label = { Text(label) },
        keyboardOptions = KeyboardOptions(keyboardType = KeyboardType.Number)
    )
}
