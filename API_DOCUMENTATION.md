# La-Player (Garlic Player) REST API Documentation

The player exposes a local HTTP REST API mapped under the `/v2/` path. All API endpoints return data in `application/json` format.

## Authentication (OAuth 2.0)

Most endpoints require an `access_token` to be passed as a request parameter. To obtain this token, you must first authenticate using the default credentials.

### `POST /v2/oauth2/token`

**Description:** Retrieves a Bearer token for accessing protected endpoints.

**Request Body (application/json) or Form-Data:**
```json
{
  "grant_type": "password",
  "username": "admin",
  "password": ""
}
```
*(Note: `admin` and an empty password are the default credentials unless modified in the player's core configuration).*

**Response Example:**
```json
{
  "access_token": "your_generated_token_string",
  "token_type": "Bearer",
  "expires_in": "3600"
}
```

---

## System Information Endpoints

These endpoints retrieve details about the device, the operative system (OS), the player's software version, and real-time hardware status such as location (GPS).

**Base Requirements for these endpoints:**
- **URL Parameter:** `?access_token=your_generated_token_string`

### `GET /v2/system/firmwareInfo`

**Description:** Returns the software version of the player and the device family (which includes the OS).

**Response Example:**
```json
{
  "firmwareVersion": "v0.6.0.671",
  "family": "La-Player-android"
}
```
- `firmwareVersion`: The current build version of the player.
- `family`: A combination of the application name and the underlying operative system (e.g., `android`, `linux`, `windows`).

### `GET /v2/system/modelInfo`

**Description:** Returns detailed model configuration, including the license model which specifies the underlying Operating System.

**Response Example:**
```json
{
  "modelDescription": "",
  "modelName": "La-Player",
  "modelURL": "",
  "manufacturer": "Sagiadinos",
  "licenseModel": "android",
  "PCBRevision": "",
  "manufacturerURL": "https://garlic-player.com",
  "PCB": "La-Player",
  "options": ""
}
```
- `modelName` / `PCB`: The branded name of the application (e.g., La-Player).
- `licenseModel`: Represents the Operative System the player is currently running on (e.g., `android`).

### `GET /v2/system/gpsInfo`

**Description:** Returns the current real-time GPS location of the device if running on Android with location permissions granted.

**Response Example:**
```json
{
  "latitude": "52.520008",
  "longitude": "13.404954"
}
```
*(Note: Returns `"n/a"` for latitude and longitude if location services are disabled, unavailable, or permissions are not granted).*

---

## Task Management Endpoints

**Base Requirements:**
- **URL Parameter:** `?access_token=your_generated_token_string`

### `GET /v2/task/reboot`

**Description:** Triggers a system-level reboot of the device.

**Response Example:**
*(Empty Response on success)*

---

## Quick Start Example Workflow

Assuming the player is running on `http://192.168.1.100:8080`, a typical workflow using `curl` would be:

1. **Get the Token:**
   ```bash
   curl -X POST http://192.168.1.100:8080/v2/oauth2/token -H "Content-Type: application/json" -d '{"grant_type":"password", "username":"admin", "password":""}'
   ```

2. **Fetch Device Location (GPS):**
   ```bash
   # Use the token received from the previous step
   curl -X GET "http://192.168.1.100:8080/v2/system/gpsInfo?access_token=YOUR_TOKEN"
   ```

3. **Reboot the Device remotely:**
   ```bash
   curl -X GET "http://192.168.1.100:8080/v2/task/reboot?access_token=YOUR_TOKEN"
   ```
