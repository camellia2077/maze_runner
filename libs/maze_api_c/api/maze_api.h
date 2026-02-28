#ifndef MAZE_API_C_MAZE_API_H
#define MAZE_API_C_MAZE_API_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MazeHandle MazeHandle;
typedef void (*MazeFrameCallback)(const unsigned char* rgba_data,
                                  size_t rgba_size, int width, int height,
                                  int channels, void* user_data);

enum MazeStatusCode {
  MAZE_STATUS_OK = 0,
  MAZE_STATUS_INVALID_ARGUMENT = 1,
  MAZE_STATUS_NOT_IMPLEMENTED = 2,
  MAZE_STATUS_INTERNAL_ERROR = 3,
};

const char* MazeApiVersion(void);

int MazeCreateHandle(MazeHandle** out_handle);
void MazeDestroyHandle(MazeHandle* handle);
const char* MazeGetLastError(const MazeHandle* handle);
const char* MazeGetSummaryJson(const MazeHandle* handle);

int MazeSetFrameCallback(MazeHandle* handle, MazeFrameCallback callback,
                         void* user_data);
int MazeRunFromJson(MazeHandle* handle, const char* request_json);

int MazeRun(MazeHandle* handle);
int MazeGetFrameCount(const MazeHandle* handle, int* out_count);
int MazeGetFrameRgba(const MazeHandle* handle, int frame_index,
                     unsigned char* out_rgba, size_t out_rgba_size,
                     int* out_width, int* out_height);

#ifdef __cplusplus
}
#endif

#endif  // MAZE_API_C_MAZE_API_H
