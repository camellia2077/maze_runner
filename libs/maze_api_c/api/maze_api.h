#ifndef MAZE_API_C_MAZE_API_H
#define MAZE_API_C_MAZE_API_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MazeSession MazeSession;

enum MazeStatusCode {
  MAZE_STATUS_OK = 0,
  MAZE_STATUS_INVALID_ARGUMENT = 1,
  MAZE_STATUS_NOT_IMPLEMENTED = 2,
  MAZE_STATUS_INTERNAL_ERROR = 3,
  MAZE_STATUS_CANCELLED = 4,
};

enum MazeEventType {
  MAZE_EVENT_RUN_STARTED = 0,
  MAZE_EVENT_CELL_STATE_CHANGED = 1,
  MAZE_EVENT_PATH_UPDATED = 2,
  MAZE_EVENT_PROGRESS = 3,
  MAZE_EVENT_RUN_FINISHED = 4,
  MAZE_EVENT_RUN_CANCELLED = 5,
  MAZE_EVENT_RUN_FAILED = 6,
  MAZE_EVENT_TOPOLOGY_SNAPSHOT = 7,
};

typedef struct MazeEvent {
  uint64_t seq;
  int type;
  const void* payload;
  size_t payload_size;
} MazeEvent;

typedef void (*MazeEventCallback)(const MazeEvent* event, void* user_data);

const char* MazeApiVersion(void);

int MazeSessionCreate(MazeSession** out_session);
void MazeSessionDestroy(MazeSession* session);

int MazeSessionConfigure(MazeSession* session, const char* request_json);
int MazeSessionSetEventCallback(MazeSession* session,
                                MazeEventCallback callback, void* user_data);
int MazeSessionRun(MazeSession* session);
int MazeSessionCancel(MazeSession* session);

int MazeSessionGetSummaryJson(const MazeSession* session, const char** out_json);
int MazeSessionGetLastError(const MazeSession* session, const char** out_error);

#ifdef __cplusplus
}
#endif

#endif  // MAZE_API_C_MAZE_API_H
