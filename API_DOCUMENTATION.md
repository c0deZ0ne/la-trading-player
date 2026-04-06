# Garlic Player - REST API Documentation (v2)

The Garlic Player exposes a local REST API for device management, file synchronization, and playback control.

## Base URL
`http://<device-ip>:<port>/v2`

> [!NOTE]
> Most endpoints (except for authentication) require a valid `access_token` passed as a query parameter.

---

## 1. Authentication

### Get Access Token
Obtain a token for authorized API calls.

- **Endpoint**: `/oauth2/token`
- **Method**: `POST`
- **Request Body (JSON)**:
  ```json
  {
    "grant_type": "password",
    "username": "your_username",
    "password": "your_password"
  }
  ```
- **Response**: Returns a JSON object containing the `access_token`.

---

## 2. System Information

### Get Firmware Info
- **Endpoint**: `/system/firmwareInfo`
- **Method**: `GET`

### Get Model Info
- **Endpoint**: `/system/modelInfo`
- **Method**: `GET`

### Get GPS Info
- **Endpoint**: `/system/gpsInfo`
- **Method**: `GET`

---

## 3. File Management

### Upload New File
- **Endpoint**: `/files/new`
- **Method**: `POST`
- **Query Parameters**: `fileSize`, `downloadPath`, `etag`, `mimeType`, `modifiedDate`
- **Form Data**: `data` (binary file content)

### List and Search Files
- **Endpoint**: `/files/find`
- **Method**: `GET` / `POST`
- **Parameters (Query or JSON)**: `maxResults`, `pageToken`
- **Description**: Returns a paginated list of files stored in the player cache.

### Delete File
- **Endpoint**: `/files/delete`
- **Method**: `POST`
- **Parameters (Query or JSON)**: `id` (The file ID or path)

### Get/Modify File by ID
- **Endpoint**: `/files/{id}`
- **Methods**: 
  - `GET`: Retrieve metadata for a specific file.
  - `POST`: Update/Resume file upload. (Supports `seek` parameter for partial uploads).

---

## 4. Application & Playback Control

### Execute Temporary URI
- **Endpoint**: `/app/exec`
- **Method**: `POST`
- **Parameters (Query or JSON)**: `uri`, `packageName`, `className`, `Action`, `Type`
- **Description**: Plays the provided URI/App immediately without changing the device's default startup content.

### Start Content (Update Home)
- **Endpoint**: `/app/start`
- **Method**: `POST`
- **Parameters (Query or JSON)**: `uri`, `packageName`, `className`
- **Description**: Sets the provided URI as the device's main content and switches to playback.

### Switch Playback Mode
- **Endpoint**: `/app/switch`
- **Method**: `POST`
- **Parameters (Query or JSON)**: `mode` (currently only `"start"` is supported)
- **Description**: Commands the player to return to its primary SMIL parsing engine.

---

## 5. Device Tasks

### Send SMIL Notification
- **Endpoint**: `/task/notify`
- **Method**: `POST`
- **Parameters (Query or JSON)**: `smilEvent`
- **Description**: Injects an event into the SMIL runtime (useful for interactive triggers).

### Reboot Device
- **Endpoint**: `/task/reboot`
- **Method**: `POST`

### Capture Screenshot
- **Endpoint**: `/task/screenshot`
- **Method**: `GET`
- **Response**: Returns a live JPEG screenshot of the current player output.

---

## 6. Access Local Cache

### Static File Access
- **Endpoint**: `/cache/*`
- **Method**: `GET`
- **Description**: Direct HTTP access to any file stored in the local `garlic-player` cache directory.
