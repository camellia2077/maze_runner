# JSON Contract Samples

This folder stores shared JSON request contracts used by different presentation layers.

- `json_request_smoke_v1.json`: minimal smoke request (`schema_version=1`) intended for:
  - C API unit smoke
  - Windows GUI manual smoke
  - Android JNI/Compose smoke

Goal:
- Keep one canonical request payload across CLI/Win/Android validation flows.
