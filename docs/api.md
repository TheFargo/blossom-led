# HTTP API: Blossom Programmable Light Display

When you use your web browser to [play animations](docs/animation_guide.md) or [kick off a meditation](docs/meditations.md), your device communicates with Blossom using `HTTP` ("Hypertext Transfer Protocol.") This is the language computers use when they're sending web pages back and forth, making it the "lingua franca" of the Internet. 

Every button or slider on the control webpage sends a little HTTP request to the Blossom and gets a small reply back.

This page describes how to format those requests, so that any device on your network can send instructions to the Blossom. The possibilities are endless: you can synchronize your Blossom to the colors on your TV, control it via a phone shortcut or home automation hub, use it to see the status of your servers, or even train your AI Agent to use Blossom to show when it's thinking or dreaming.

---

## Table of Contents

1. [What Is an "HTTP API?"](#1-what-is-an-http-api)
2. [Before You Start](#2-before-you-start)
3. [Trying It Out with curl](#3-trying-it-out-with-curl)
4. [Status Endpoints](#4-status-endpoints)
5. [Controlling the LEDs](#5-controlling-the-leds)
6. [Animation Endpoints](#6-animation-endpoints)
7. [Preset Endpoints](#7-preset-endpoints)
8. [Meditation Endpoints](#8-meditation-endpoints)
9. [Errors and Troubleshooting](#9-errors-and-troubleshooting)
10. [Next Steps and Further Reading](#10-next-steps-and-further-reading)

---

# 1. What Is an "HTTP API?"

Every time your web browser loads a page, it sends an *HTTP request* to a server and gets an *HTTP response* back. This is the core mechanic for the entire web. 

An "API" (Application Programming Interface) is essentially an "instruction manual" listing all the possible requests and responses and what they mean and do. Blossom runs its own web server, so you can ask it to show you web pages - this is what happens when you connect to `blossom.local.` In addition to serving ordinary web pages, Blossom has its own special instructions it understands, such as:

* "What's your status?" Blossom will respond with its current state and what animations its playing. 
* "Toggle the LEDs" Blossom will toggle its LEDs on or off and confirm.
* "Start a 60-second meditation." Blossom will send a confirmation, flip to meditation mode and begin the sequence.

These little information requests are sent back and forth in a format called "JSON." 

### A quick JSON primer

JSON is just a lightweight text format for describing data, using curly braces `{}` for objects and square brackets `[]` for lists. For example, this is what Blossom sends back when you ask for its status:

```json
{"status":"connected","ssid":"MyHomeWifi","ip":"192.168.1.42","rssi":-58}
```

That's a JSON *object* with four *keys* (`status`, `ssid`, `ip`, `rssi`) and their corresponding values. The nice thing about JSON is that it's "human radable" - in other words, you can look at that data packet and everything is labelled. 

---

# 2. Before You Start

- **Blossom must be connected to your WiFi network** (not in its own provisioning setup mode). If you can load `http://blossom.local` in a browser and see the status page, you're ready.
- All the examples below use the hostname `blossom.local`. If mDNS (the technology behind `.local` addresses) isn't working on your network for some reason, use Blossom's IP address instead. You can find it from the `/api/status` endpoint below, from the status page, or from your router's connected-devices list.
- Nothing here requires an account, API key, or login. This API is designed for a private home network.

>[!NOTE]
>To underscore that last point, Blossom is an open device. It'll handle any request it receives on its local network. If it were somehow connected to the open Internet, any Black Hat from Sheboygan who knew your Blossom's IP address could change the colors on you.

---

# 3. Trying It Out with curl

There's a command-line tool called `curl` built into macOS, Linux, and modern Windows that lets you directly send HTTP requests. Open a terminal (Command Prompt, PowerShell, or Terminal) and try:

```bash
curl http://blossom.local/api/status
```

You should get back something like:

```json
{"status":"connected","ssid":"MyHomeWifi","ip":"192.168.1.42","rssi":-58}
```

That's a *GET* request. It's just asking Blossom for information in a read-only operation. Some endpoints instead require a *POST* request, which means you're sending Blossom some data and asking it to _do_ something with it. With curl, a POST with a JSON body looks like this:

```bash
curl -X POST http://blossom.local/api/led/toggle
```

The above line toggles the LEDs on or off. For endpoints that need data in addition to the request, you might use a line like this:

```bash
curl -X POST http://blossom.local/api/leds -H "Content-Type: application/json" -d "{\"enabled\":true}"
```

The above line connects to the "leds" endpoint, specifies that we'll send the data in JSON format, and sends the JSON code to set "enabled" to "true." Instead of toggling the LEDs, this line will turn them on, or leave them that way if they're already on.

> [!TIP]
> Graphical apps like [Postman](https://www.postman.com/) or the free [Insomnia](https://insomnia.rest/) let you build and send requests by filling out a form instead of typing commands. Either way, the same commands are sent to Blossom.

---

# 4. Status Endpoints

### `GET /api/status`

Returns basic connection info: current WiFi network, IP address, and signal strength.

```bash
curl http://blossom.local/api/status
```

```json
{"status":"connected","ssid":"MyHomeWifi","ip":"192.168.1.42","rssi":-58}
```

`rssi` is signal strength in dBm. Closer to 0 is a stronger signal. Typical WiFi ranges from about -30 (excellent) to -80 (weak).

---

# 5. Controlling the LEDs

These endpoints turn the entire LED ring on or off.

### `GET /api/led`

Returns whether the LEDs are currently on.

```bash
curl http://blossom.local/api/led
```
```json
{"led":false}
```

### `POST /api/led/toggle`

Flips the LEDs from on→off or off→on. No request body needed.

```bash
curl -X POST http://blossom.local/api/led/toggle
```
```json
{"led":true}
```

### `POST /api/leds`

Explicitly sets the LEDs on or off (rather than toggling).

```bash
curl -X POST http://blossom.local/api/leds -H "Content-Type: application/json" -d "{\"enabled\":true}"
```

```json
{"enabled": true}
```

---

# 6. Animation Endpoints

This API controls color, sparkles, flicker, pulse, and spin, using the same parameters described in the [Animation Guide](/docs/animation_guide.md).

### `GET /api/animation`

Returns the animation configuration currently playing, plus the display name of the active preset (or `"Custom"` if it's been edited).

```bash
curl http://blossom.local/api/animation
```

```json
{
  "name": "Warm Flame",
  "color.primary": 21,
  "color.spread": 25,
  "color.brightness": 200,
  "color.mode": 1,
  "sparkles.brightness": 100,
  "sparkles.spread": 80,
  "sparkles.mode": 1,
  "flicker.apply_to_color": true,
  "flicker.apply_to_sparkles": true,
  "flicker.speed": 60,
  "flicker.amplitude": 120,
  "flicker.mode": 1,
  "pulse.apply_to_color": true,
  "pulse.apply_to_sparkles": false,
  "pulse.speed": 30,
  "pulse.amplitude": 80,
  "pulse.mode": 3,
  "spin.apply_to_color": false,
  "spin.apply_to_sparkles": false,
  "spin.speed": 0
}
```

### `POST /api/animation`

Sends a full animation configuration to the Blossom and applies it immediately. After a successful call, the "Now Playing" preset name becomes `"Custom"` (see the [Animation Guide](/docs/animation_guide.md#5-saving-presets) for how naming and presets interact).

>[!TIP]
>This is the command that's sent every time you play with the controls on Blossom's animation webpage.

*Every field below is required*. This endpoint expects the whole object, not just the fields you want to change. The easiest way to build a valid body is to `GET /api/animation` first, tweak the fields you care about, and `POST` the result back.

| Field | Type | Range | Meaning |
|---|---|---|---|
| `color.primary` | integer | 0–255 | Primary hue (maps to a 0–359° color wheel position) |
| `color.spread` | integer | 0–255 | How far the color wanders from the primary hue |
| `color.brightness` | integer | 0–255 | Brightness of the color LEDs |
| `color.mode` | integer | 0–3 | Distribution mode — see table below |
| `sparkles.brightness` | integer | 0–255 | Base brightness of the white sparkle LEDs |
| `sparkles.spread` | integer | 0–255 | How much sparkle brightness varies |
| `sparkles.mode` | integer | 0–3 | Distribution mode — see table below |
| `flicker.apply_to_color` | boolean | — | Apply the flicker animation to the color channel |
| `flicker.apply_to_sparkles` | boolean | — | Apply the flicker animation to the sparkle channel |
| `flicker.speed` | integer | 0–255 | How fast the flicker noise pattern moves |
| `flicker.amplitude` | integer | 0–255 | How strong the flicker effect is |
| `flicker.mode` | integer | 0–3 | Synchronicity across the ring — see table below |
| `pulse.apply_to_color` | boolean | — | Apply the pulse animation to the color channel |
| `pulse.apply_to_sparkles` | boolean | — | Apply the pulse animation to the sparkle channel |
| `pulse.speed` | integer | 0–255 | How fast the sine-wave pulse cycles |
| `pulse.amplitude` | integer | 0–255 | How strong the pulse effect is |
| `pulse.mode` | integer | 0–3 | Synchronicity across the ring — see table below |
| `spin.apply_to_color` | boolean | — | Spin the color channel around the ring |
| `spin.apply_to_sparkles` | boolean | — | Spin the sparkle channel around the ring |
| `spin.speed` | integer | -128–127 | Spin speed and direction (negative = reverse) |

**Distribution modes** (used by `color.mode`, `sparkles.mode`, `flicker.mode`, and `pulse.mode`):

| Value | Name | Meaning |
|---|---|---|
| `0` | Unison | All 16 LEDs act identically |
| `1` | Random | Spread in a crisscross pattern around the ring |
| `2` | Ordered | Spread evenly in sequence around the ring |
| `3` | Looping | Mirrored on both halves of the ring for a seamless loop |

```bash
curl -X POST http://blossom.local/api/animation ^
  -H "Content-Type: application/json" ^
  -d "{\"color.primary\":21,\"color.spread\":25,\"color.brightness\":200,\"color.mode\":1,\"sparkles.brightness\":100,\"sparkles.spread\":80,\"sparkles.mode\":1,\"flicker.apply_to_color\":true,\"flicker.apply_to_sparkles\":true,\"flicker.speed\":60,\"flicker.amplitude\":120,\"flicker.mode\":1,\"pulse.apply_to_color\":true,\"pulse.apply_to_sparkles\":false,\"pulse.speed\":30,\"pulse.amplitude\":80,\"pulse.mode\":3,\"spin.apply_to_color\":false,\"spin.apply_to_sparkles\":false,\"spin.speed\":0}"
```

> [!TIP]
> Don't want to hand-craft that whole object? Open the animation page in your browser, dial in the look you want using the sliders, then call `GET /api/animation` to fetch exactly the JSON that produced it. Save that as a template for your own scripts.

---

# 7. Preset Endpoints

Presets are named, saved animation configurations (see [Saving Presets](/docs/animation_guide.md#5-saving-presets) in the Animation Guide). These endpoints let you list, save, and load presets remotely.

### `GET /api/presets`

Lists every saved preset, which one is currently playing, and which one is set to load automatically on boot. The built-in "Warm Flame" preset is always included, even on a brand-new device.

```bash
curl http://blossom.local/api/presets
```

```json
{"current":"Warm Flame","default":"Warm Flame","presets":["Warm Flame","BluSwirl","Rainbow Slo"]}
```

### `POST /api/presets/save`

Saves the given animation configuration under a name. Accepts the same fields as `POST /api/animation` (see the table above), plus two extra ones:

| Field | Type | Meaning |
|---|---|---|
| `name` | string | Display name for the preset (max 24 characters). Saving under an existing name overwrites it. |
| `makeDefault` | boolean | If `true`, this preset will load automatically the next time Blossom powers on. |

```bash
curl -X POST http://blossom.local/api/presets/save ^
  -H "Content-Type: application/json" ^
  -d "{\"name\":\"Ocean Breeze\",\"makeDefault\":false,\"color.primary\":150,\"color.spread\":40,\"color.brightness\":180,\"color.mode\":1,\"sparkles.brightness\":60,\"sparkles.spread\":50,\"sparkles.mode\":1,\"flicker.apply_to_color\":true,\"flicker.apply_to_sparkles\":false,\"flicker.speed\":40,\"flicker.amplitude\":90,\"flicker.mode\":1,\"pulse.apply_to_color\":false,\"pulse.apply_to_sparkles\":false,\"pulse.speed\":0,\"pulse.amplitude\":0,\"pulse.mode\":0,\"spin.apply_to_color\":false,\"spin.apply_to_sparkles\":false,\"spin.speed\":0}"
```

```json
{"status":"saved","name":"Ocean Breeze"}
```

A maximum of 16 presets can be saved at once. Saving a 17th (under a brand-new name) will fail.

### `POST /api/presets/load`

Loads a saved preset by name and applies it to the LEDs immediately.

```bash
curl -X POST http://blossom.local/api/presets/load -H "Content-Type: application/json" -d "{\"name\":\"Ocean Breeze\"}"
```

The response is the full config JSON of the preset that was just loaded (same shape as `GET /api/animation`).

---

# 8. Meditation Endpoints

These control Blossom's guided-breathing Meditation Mode, described in the [Meditation Guide](/docs/meditations.md). Functionally, it temporarily takes over the LEDs to run a breathing cycle (inhale / hold / exhale / hold), independent of whatever animation was playing before.

### `POST /api/meditation/start`

Starts a meditation session.

```bash
curl -X POST http://blossom.local/api/meditation/start -H "Content-Type: application/json" -d "{\"duration\":60}"
```

`duration` is in seconds. Use `0` for an open-ended session that runs until manually stopped.

### `POST /api/meditation/stop`

Ends the current session immediately (no closing light sequence; it just stops).

```bash
curl -X POST http://blossom.local/api/meditation/stop
```

### `GET /api/meditation/status`

Reports what phase of the breathing cycle is currently active, useful if you're building your own on-screen guidance to match the lights.

```bash
curl http://blossom.local/api/meditation/status
```

```json
{"active":true,"phase":"inhale","phaseSecondsLeft":2,"sessionSecondsElapsed":14,"sessionSecondsRemaining":46}
```

`phase` will be one of `"inhale"`, `"hold"`, `"exhale"`, or `"idle"`. "Idle" is returned when mediation mode isn't active. 

---

# 9. Errors and Troubleshooting

- **A request seems to hang or time out:** Double-check `blossom.local` resolves on your network — try Blossom's raw IP address instead (from your router, or from `/api/status` while you still have another way to reach it).
- **You get `{"error":"No data received"}`:** The endpoint expected a JSON body (a `POST` with data) but didn't get one. Make sure you're sending a `Content-Type: application/json` header and a valid `-d` body.
- **A `POST /api/presets/load` returns 404 with `{"error":"Preset not found"}`:** Double check the exact spelling of the name you're loading — check `GET /api/presets` for the exact list of valid names.
- **You changed something but the web page still shows old values:** The web page only reads the current state when it loads. Refresh it, or call `GET /api/animation` yourself to confirm the change actually landed.
- **Note on CORS:** Every response includes `Access-Control-Allow-Origin: *`, so you can call these endpoints directly from JavaScript running on another webpage (for example, a custom dashboard) without running into cross-origin browser restrictions.

---

# 10. Next Steps and Further Reading

This API is intentionally simple, a great way to try your hand at some lightweight home automation or IoT projects:

- A phone home-screen shortcut that turns Blossom on/off with one tap
- A script that changes Blossom's colors to match the weather, the time of day, or your calendar
- Integration with a home automation hub (Home Assistant, openHAB, Node-RED, etc.) using generic HTTP request nodes
- Integration with an LLM Agent to display its current status.

If you build something cool with the Blossom, don't keep it to yourself. Share it with the open-source community! We can't wait to see it!

### Blossom Documentation

* **[LED Technical Details](/docs/led_technical_details.md):** All about the LED lights used in this project.
* **[Animation Guide](/docs/animation_guide.md):** Description of Blossom's Animation parameters.
* **[Meditation Guide](/docs/meditations.md):** Description of Blossom's meditation exercise.

### Web & API Fundamentals

* **[MDN Web Docs: An Overview of HTTP](https://developer.mozilla.org/en-US/docs/Web/HTTP/Overview):** Covers client-server architecture, HTTP methods (`GET`, `POST`, `PUT`), status codes, and header structures.
* **[Postman Learning Center](https://learning.postman.com/):** A beginner-friendly introduction to testing API endpoints, formatting JSON request bodies, and observing server responses without writing frontend code.
* **[RESTful API Design Primer (REST API Tutorial)](https://restfulapi.net/):** An accessible overview of REST constraints, request payload conventions, and resource-oriented architecture.
* **[Adafruit Learning System: Welcome to Adafruit IO](https://learn.adafruit.com/welcome-to-adafruit-io):** Illustrates how hardware devices communicate with web APIs, manage network state, and interact with cloud feeds.

### Real-Time IoT Communication Protocols

* **[HiveMQ MQTT Essentials](https://www.hivemq.com/mqtt/):** Introduces lightweight publish-subscribe protocols, useful for readers curious about alternatives to HTTP REST for high-frequency or real-time light syncing.
* **[MDN Web Docs: WebSockets API](https://developer.mozilla.org/en-US/docs/Web/API/WebSockets_API):** Explains full-duplex communication channels for persistent connections and low-latency status updates.
