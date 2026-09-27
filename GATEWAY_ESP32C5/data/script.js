// ============================================================
// SINGFAST ELECTRIC CAR
// WEB CONTROL
// ============================================================


// ============================================================
// ESP32 CONNECTION
// ============================================================

// Đổi địa chỉ này thành WebSocket server của ESP32.
//
// Ví dụ:
// ws://192.168.4.1:81
//
// Nếu ESP32 dùng IP khác thì sửa dòng này.

const ESP32_WS_URL = "ws://192.168.4.1:81";


// Timeout kết nối

const CONNECTION_TIMEOUT = 5000;


// Khoảng thời gian ping STM32

const INTERNAL_PING_INTERVAL = 500;


// ============================================================
// GLOBAL STATE
// ============================================================

let socket = null;

let connectionTimer = null;

let internalLatencyTimer = null;

let lastInternalPing = 0;

let internalLatency = Infinity;

let selectedTurnAngle = 30;


// ============================================================
// KEY STATE
// ============================================================

const keys = {

    w: false,
    a: false,
    s: false,
    d: false

};


// ============================================================
// GET HTML ELEMENTS
// ============================================================

const btnForward =
    document.getElementById("btn-forward");

const btnBackward =
    document.getElementById("btn-backward");

const btnLeft =
    document.getElementById("btn-left");

const btnRight =
    document.getElementById("btn-right");

const btnStop =
    document.getElementById("btn-stop");


const angleButtons =
    document.querySelectorAll(".angle-button");


const selectedAngleText =
    document.getElementById("selected-angle");


const connectionStatus =
    document.getElementById("connection-status");


const statusDot =
    document.getElementById("status-dot");


const internalLatencyElement =
    document.getElementById("internal-latency");


const robotStateElement =
    document.getElementById("robot-state");


const systemMessageElement =
    document.getElementById("system-message");


const statusMessageBox =
    document.querySelector(".status-message");


const telemetryAngle =
    document.getElementById("telemetry-angle");


const telemetryYaw =
    document.getElementById("telemetry-yaw");


const telemetryLeftSpeed =
    document.getElementById("telemetry-left-speed");


const telemetryRightSpeed =
    document.getElementById("telemetry-right-speed");


const batteryValue =
    document.getElementById("battery-value");


// ============================================================
// CONTROL BUTTON MAP
// ============================================================

const buttons = {

    w: btnForward,
    a: btnLeft,
    s: btnBackward,
    d: btnRight

};


// ============================================================
// CONNECTION STATUS
// ============================================================

function setConnectionStatus(status) {

    statusDot.classList.remove(
        "connected",
        "connecting",
        "disconnected"
    );


    connectionStatus.classList.remove(
        "connected",
        "connecting",
        "disconnected"
    );


    if (status === "CONNECTED") {

        statusDot.classList.add("connected");

        connectionStatus.classList.add("connected");

        connectionStatus.textContent =
            "CONNECTED";

    }


    else if (status === "CONNECTING") {

        statusDot.classList.add("connecting");

        connectionStatus.classList.add("connecting");

        connectionStatus.textContent =
            "CONNECTING";

    }


    else {

        statusDot.classList.add("disconnected");

        connectionStatus.classList.add("disconnected");

        connectionStatus.textContent =
            "DISCONNECTED";

    }

}


// ============================================================
// MESSAGE
// ============================================================

function setSystemMessage(
    message,
    type = "normal"
) {

    systemMessageElement.textContent =
        message;


    statusMessageBox.classList.remove(
        "error",
        "warning",
        "success"
    );


    if (type === "error") {

        statusMessageBox.classList.add("error");

    }


    else if (type === "warning") {

        statusMessageBox.classList.add("warning");

    }


    else if (type === "success") {

        statusMessageBox.classList.add("success");

    }

}


// ============================================================
// CONNECTION
// ============================================================

function connectESP32() {

    clearTimeout(connectionTimer);


    setConnectionStatus("CONNECTING");

    setInternalLatency(Infinity);

    setSystemMessage(
        "Connecting...",
        "warning"
    );


    try {

        socket =
            new WebSocket(ESP32_WS_URL);

    }

    catch (error) {

        setConnectionStatus("DISCONNECTED");

        setSystemMessage(
            "Error: Connection failed",
            "error"
        );

        return;
    }


    // --------------------------------------------
    // OPEN
    // --------------------------------------------

    socket.onopen = function () {

        clearTimeout(connectionTimer);


        setConnectionStatus("CONNECTED");


        setSystemMessage(
            "Connected to ESP32",
            "success"
        );


        startInternalLatency();


        sendInitialStatusRequest();

    };


    // --------------------------------------------
    // MESSAGE
    // --------------------------------------------

    socket.onmessage = function (event) {

        handleIncomingMessage(
            event.data
        );

    };


    // --------------------------------------------
    // ERROR
    // --------------------------------------------

    socket.onerror = function () {

        setSystemMessage(
            "Error: Connection error",
            "error"
        );

    };


    // --------------------------------------------
    // CLOSE
    // --------------------------------------------

    socket.onclose = function () {

        stopInternalLatency();


        setConnectionStatus(
            "DISCONNECTED"
        );


        setInternalLatency(
            Infinity
        );


        setSystemMessage(
            "Error: Connection lost",
            "error"
        );


        socket = null;

    };


    // --------------------------------------------
    // CONNECTION TIMEOUT
    // --------------------------------------------

    connectionTimer = setTimeout(
        function () {

            if (
                socket &&
                socket.readyState !==
                WebSocket.OPEN
            ) {

                try {

                    socket.close();

                }

                catch (error) {

                    // Nothing to do
                }


                setConnectionStatus(
                    "DISCONNECTED"
                );


                setInternalLatency(
                    Infinity
                );


                setSystemMessage(
                    "Error: Connection timeout",
                    "error"
                );

            }

        },
        CONNECTION_TIMEOUT
    );

}


// ============================================================
// SEND JSON
// ============================================================

function sendJSON(data) {

    if (
        !socket ||
        socket.readyState !== WebSocket.OPEN
    ) {

        setConnectionStatus(
            "DISCONNECTED"
        );

        setInternalLatency(
            Infinity
        );

        return false;

    }


    socket.send(
        JSON.stringify(data)
    );

    return true;

}


// ============================================================
// INITIAL STATUS REQUEST
// ============================================================

function sendInitialStatusRequest() {

    sendJSON({

        type: "status_request"

    });

}


// ============================================================
// INTERNAL LATENCY
// ============================================================
//
// ESP32 → STM32 → ESP32
//
// Web sends request to ESP32.
// ESP32 forwards PING to STM32.
// STM32 returns ACK.
// ESP32 sends response back to Web.
//
// The Web measures the complete round-trip.
//
// ============================================================

function startInternalLatency() {

    stopInternalLatency();


    internalLatencyTimer =
        setInterval(
            function () {

                if (
                    !socket ||
                    socket.readyState !==
                    WebSocket.OPEN
                ) {

                    setInternalLatency(
                        Infinity
                    );

                    return;

                }


                lastInternalPing =
                    performance.now();


                sendJSON({

                    type: "internal_ping"

                });

            },
            INTERNAL_PING_INTERVAL
        );

}


function stopInternalLatency() {

    if (internalLatencyTimer) {

        clearInterval(
            internalLatencyTimer
        );

        internalLatencyTimer = null;

    }

}


// ============================================================
// SET INTERNAL LATENCY
// ============================================================

function setInternalLatency(value) {

    internalLatency = value;


    if (!Number.isFinite(value)) {

        internalLatencyElement.textContent =
            "∞ ms";

        internalLatencyElement.className =
            "latency-value offline";

        return;

    }


    const rounded =
        Math.round(value * 10) / 10;


    internalLatencyElement.textContent =
        `${rounded} ms`;


    if (value < 30) {

        internalLatencyElement.className =
            "latency-value good";

    }

    else if (value < 100) {

        internalLatencyElement.className =
            "latency-value warning";

    }

    else {

        internalLatencyElement.className =
            "latency-value bad";

    }

}


// ============================================================
// INCOMING MESSAGE
// ============================================================

function handleIncomingMessage(rawData) {

    let data;


    try {

        data =
            typeof rawData === "string"
                ? JSON.parse(rawData)
                : rawData;

    }

    catch (error) {

        console.warn(
            "Invalid JSON:",
            rawData
        );

        return;

    }


    // ========================================================
    // INTERNAL PONG
    // ========================================================

    if (
        data.type === "internal_pong" ||
        data.type === "internal_ack"
    ) {

        if (lastInternalPing > 0) {

            const latency =
                performance.now() -
                lastInternalPing;

            setInternalLatency(
                latency
            );

        }

        return;

    }


    // ========================================================
    // STATUS
    // ========================================================

    if (data.type === "status") {

        if (data.state !== undefined) {

            setRobotState(
                data.state
            );

        }


        if (data.message !== undefined) {

            setSystemMessage(
                data.message
            );

        }


        return;

    }


    // ========================================================
    // TELEMETRY
    // ========================================================

    if (data.type === "telemetry") {

        updateTelemetry(data);

        return;

    }


    // ========================================================
    // ROBOT STATE
    // ========================================================

    if (data.type === "robot_state") {

        if (data.state !== undefined) {

            setRobotState(
                data.state
            );

        }


        if (data.message !== undefined) {

            setSystemMessage(
                data.message
            );

        }


        return;

    }


    // ========================================================
    // GENERIC ERROR
    // ========================================================

    if (data.type === "error") {

        setSystemMessage(
            data.message ||
            "Error",
            "error"
        );

    }

}


// ============================================================
// ROBOT STATE
// ============================================================

function setRobotState(state) {

    robotStateElement.textContent =
        String(state).toUpperCase();

}


// ============================================================
// TELEMETRY
// ============================================================

function updateTelemetry(data) {


    // --------------------------------------------
    // BALANCE ANGLE
    // --------------------------------------------

    if (Number.isFinite(data.angle)) {

        telemetryAngle.textContent =
            `${data.angle.toFixed(2)}°`;

    }


    // --------------------------------------------
    // YAW / TURN ANGLE
    // --------------------------------------------

    if (Number.isFinite(data.yaw)) {

        telemetryYaw.textContent =
            `${data.yaw.toFixed(2)}°`;

    }


    // --------------------------------------------
    // LEFT SPEED
    // --------------------------------------------

    if (
        Number.isFinite(data.leftSpeed)
    ) {

        telemetryLeftSpeed.textContent =
            `${Math.round(data.leftSpeed)} RPM`;

    }


    // --------------------------------------------
    // RIGHT SPEED
    // --------------------------------------------

    if (
        Number.isFinite(data.rightSpeed)
    ) {

        telemetryRightSpeed.textContent =
            `${Math.round(data.rightSpeed)} RPM`;

    }


    // --------------------------------------------
    // BATTERY
    // --------------------------------------------

    if (
        Number.isFinite(data.battery)
    ) {

        batteryValue.textContent =
            `${Math.round(data.battery)} %`;

    }

}


// ============================================================
// TURN ANGLE
// ============================================================

angleButtons.forEach(
    function (button) {

        button.addEventListener(
            "click",
            function () {

                selectedTurnAngle =
                    Number(
                        button.dataset.angle
                    );


                angleButtons.forEach(
                    function (item) {

                        item.classList.remove(
                            "active"
                        );

                    }
                );


                button.classList.add(
                    "active"
                );


                selectedAngleText.textContent =
                    `${selectedTurnAngle}°`;

            }
        );

    }
);


// ============================================================
// UPDATE BUTTON VISUAL
// ============================================================

function updateButtonVisual(key) {

    if (!buttons[key]) {
        return;
    }


    if (keys[key]) {

        buttons[key].classList.add(
            "pressed"
        );

    }

    else {

        buttons[key].classList.remove(
            "pressed"
        );

    }

}


// ============================================================
// RELEASE ALL KEYS
// ============================================================

function releaseAllKeys() {

    keys.w = false;
    keys.a = false;
    keys.s = false;
    keys.d = false;


    updateButtonVisual("w");
    updateButtonVisual("a");
    updateButtonVisual("s");
    updateButtonVisual("d");

}


// ============================================================
// KEYBOARD DOWN
// ============================================================

document.addEventListener(
    "keydown",
    function (event) {

        const key =
            event.key.toLowerCase();


        // ----------------------------------------
        // SPACE
        // ----------------------------------------

        if (event.code === "Space") {

            event.preventDefault();


            releaseAllKeys();


            btnStop.classList.add(
                "pressed"
            );


            sendCommand(
                "STOP",
                0,
                0,
                0
            );


            return;

        }


        // ----------------------------------------
        // WASD
        // ----------------------------------------

        if (
            !["w", "a", "s", "d"]
                .includes(key)
        ) {

            return;

        }


        event.preventDefault();


        if (keys[key]) {

            return;

        }


        keys[key] = true;


        updateButtonVisual(key);


        updateCommand();

    }
);


// ============================================================
// KEYBOARD UP
// ============================================================

document.addEventListener(
    "keyup",
    function (event) {

        const key =
            event.key.toLowerCase();


        // ----------------------------------------
        // SPACE
        // ----------------------------------------

        if (event.code === "Space") {

            btnStop.classList.remove(
                "pressed"
            );

            return;

        }


        // ----------------------------------------
        // WASD
        // ----------------------------------------

        if (
            !["w", "a", "s", "d"]
                .includes(key)
        ) {

            return;

        }


        event.preventDefault();


        keys[key] = false;


        updateButtonVisual(key);


        updateCommand();

    }
);


// ============================================================
// CALCULATE ROBOT COMMAND
// ============================================================

function updateCommand() {

    const w = keys.w;

    const a = keys.a;

    const s = keys.s;

    const d = keys.d;


    let command = "STOP";

    let throttle = 0;

    let turn = 0;

    let turnAngle = 0;


    // ========================================================
    // FORWARD
    // ========================================================

    if (w && !s) {

        throttle = 100;


        if (a && !d) {

            command =
                "FORWARD_LEFT";

            turn = -100;

            turnAngle =
                selectedTurnAngle;

        }


        else if (d && !a) {

            command =
                "FORWARD_RIGHT";

            turn = 100;

            turnAngle =
                selectedTurnAngle;

        }


        else {

            command =
                "FORWARD";

        }

    }


    // ========================================================
    // BACKWARD
    // ========================================================

    else if (s && !w) {

        throttle = -100;


        if (a && !d) {

            command =
                "BACKWARD_LEFT";

            turn = -100;

            turnAngle =
                selectedTurnAngle;

        }


        else if (d && !a) {

            command =
                "BACKWARD_RIGHT";

            turn = 100;

            turnAngle =
                selectedTurnAngle;

        }


        else {

            command =
                "BACKWARD";

        }

    }


    // ========================================================
    // TURN LEFT
    // ========================================================

    else if (a && !d) {

        command =
            "TURN_LEFT";

        turn = -100;

        turnAngle = 90;

    }


    // ========================================================
    // TURN RIGHT
    // ========================================================

    else if (d && !a) {

        command =
            "TURN_RIGHT";

        turn = 100;

        turnAngle = 90;

    }


    // ========================================================
    // CONFLICT W + S
    // ========================================================

    if (w && s) {

        command = "STOP";

        throttle = 0;

        turn = 0;

        turnAngle = 0;

    }


    // ========================================================
    // CONFLICT A + D
    // ========================================================

    if (a && d) {

        command = "STOP";

        throttle = 0;

        turn = 0;

        turnAngle = 0;

    }


    // ========================================================
    // SEND
    // ========================================================

    sendCommand(
        command,
        throttle,
        turn,
        turnAngle
    );

}


// ============================================================
// SEND ROBOT COMMAND
// ============================================================

function sendCommand(
    command,
    throttle,
    turn,
    turnAngle
) {

    const packet = {

        type: "command",

        command: command,

        throttle: throttle,

        turn: turn,

        angle: turnAngle

    };


    console.log(
        "COMMAND:",
        packet
    );


    sendJSON(packet);

}


// ============================================================
// MOUSE CONTROL
// ============================================================

function setupMouseButton(
    button,
    key
) {

    button.addEventListener(
        "mousedown",
        function () {

            keys[key] = true;

            updateButtonVisual(key);

            updateCommand();

        }
    );


    button.addEventListener(
        "mouseup",
        function () {

            keys[key] = false;

            updateButtonVisual(key);

            updateCommand();

        }
    );


    button.addEventListener(
        "mouseleave",
        function () {

            if (keys[key]) {

                keys[key] = false;

                updateButtonVisual(key);

                updateCommand();

            }

        }
    );

}


// ============================================================
// SETUP MOUSE BUTTONS
// ============================================================

setupMouseButton(
    btnForward,
    "w"
);

setupMouseButton(
    btnLeft,
    "a"
);

setupMouseButton(
    btnBackward,
    "s"
);

setupMouseButton(
    btnRight,
    "d"
);


// ============================================================
// STOP BUTTON
// ============================================================

btnStop.addEventListener(
    "mousedown",
    function () {

        releaseAllKeys();


        btnStop.classList.add(
            "pressed"
        );


        sendCommand(
            "STOP",
            0,
            0,
            0
        );

    }
);


btnStop.addEventListener(
    "mouseup",
    function () {

        btnStop.classList.remove(
            "pressed"
        );

    }
);


// ============================================================
// PID PROFILES
// ============================================================

const defaultProfiles = {

    Default: {

        balance: {
            kp: 25.0,
            ki: 0.5,
            kd: 0.8
        },

        speed: {
            kp: 1.0,
            ki: 0.1,
            kd: 0.05
        },

        yaw: {
            kp: 1.0,
            ki: 0.0,
            kd: 0.1
        }

    },


    Racing: {

        balance: {
            kp: 30.0,
            ki: 0.3,
            kd: 1.0
        },

        speed: {
            kp: 1.5,
            ki: 0.1,
            kd: 0.08
        },

        yaw: {
            kp: 1.5,
            ki: 0.0,
            kd: 0.15
        }

    },


    Stable: {

        balance: {
            kp: 20.0,
            ki: 0.8,
            kd: 0.6
        },

        speed: {
            kp: 0.8,
            ki: 0.15,
            kd: 0.04
        },

        yaw: {
            kp: 0.8,
            ki: 0.0,
            kd: 0.08
        }

    },


    Test: {

        balance: {
            kp: 10.0,
            ki: 0.0,
            kd: 0.2
        },

        speed: {
            kp: 0.5,
            ki: 0.0,
            kd: 0.02
        },

        yaw: {
            kp: 0.5,
            ki: 0.0,
            kd: 0.05
        }

    }

};


// ============================================================
// LOAD SAVED PROFILES
// ============================================================

let profiles = {};

try {

    const saved =
        localStorage.getItem(
            "singfast_pid_profiles"
        );


    if (saved) {

        profiles =
            JSON.parse(saved);

    }

}

catch (error) {

    profiles = {};

}


// ============================================================
// MERGE DEFAULT PROFILES
// ============================================================

Object.keys(defaultProfiles)
    .forEach(
        function (name) {

            if (!profiles[name]) {

                profiles[name] =
                    JSON.parse(
                        JSON.stringify(
                            defaultProfiles[name]
                        )
                    );

            }

        }
    );


// ============================================================
// PROFILE ELEMENTS
// ============================================================

const profileSelect =
    document.getElementById(
        "profile-select"
    );


const activeProfileName =
    document.getElementById(
        "active-profile-name"
    );


const pidMessage =
    document.getElementById(
        "pid-message"
    );


const pidFields = {

    balanceKp:
        document.getElementById(
            "balance-kp"
        ),

    balanceKi:
        document.getElementById(
            "balance-ki"
        ),

    balanceKd:
        document.getElementById(
            "balance-kd"
        ),


    speedKp:
        document.getElementById(
            "speed-kp"
        ),

    speedKi:
        document.getElementById(
            "speed-ki"
        ),

    speedKd:
        document.getElementById(
            "speed-kd"
        ),


    yawKp:
        document.getElementById(
            "yaw-kp"
        ),

    yawKi:
        document.getElementById(
            "yaw-ki"
        ),

    yawKd:
        document.getElementById(
            "yaw-kd"
        )

};


// ============================================================
// ACTIVE PROFILE
// ============================================================

let activeProfile =
    localStorage.getItem(
        "singfast_active_profile"
    ) || "Default";


// ============================================================
// SAVE PROFILES TO LOCAL STORAGE
// ============================================================

function saveProfilesToStorage() {

    localStorage.setItem(
        "singfast_pid_profiles",
        JSON.stringify(profiles)
    );

}


// ============================================================
// REFRESH PROFILE SELECT
// ============================================================

function refreshProfileSelect() {

    const current =
        profileSelect.value;


    profileSelect.innerHTML = "";


    Object.keys(profiles)
        .forEach(
            function (name) {

                const option =
                    document.createElement(
                        "option"
                    );

                option.value = name;

                option.textContent = name;

                profileSelect.appendChild(
                    option
                );

            }
        );


    if (profiles[current]) {

        profileSelect.value =
            current;

    }

    else if (profiles[activeProfile]) {

        profileSelect.value =
            activeProfile;

    }


    updateActiveProfileText();

}


// ============================================================
// UPDATE ACTIVE PROFILE TEXT
// ============================================================

function updateActiveProfileText() {

    activeProfileName.textContent =
        `${activeProfile} ● ACTIVE`;

}


// ============================================================
// LOAD PROFILE INTO INPUTS
// ============================================================

function loadProfile(name) {

    const profile =
        profiles[name];


    if (!profile) {
        return;
    }


    pidFields.balanceKp.value =
        profile.balance.kp;


    pidFields.balanceKi.value =
        profile.balance.ki;


    pidFields.balanceKd.value =
        profile.balance.kd;


    pidFields.speedKp.value =
        profile.speed.kp;


    pidFields.speedKi.value =
        profile.speed.ki;


    pidFields.speedKd.value =
        profile.speed.kd;


    pidFields.yawKp.value =
        profile.yaw.kp;


    pidFields.yawKi.value =
        profile.yaw.ki;


    pidFields.yawKd.value =
        profile.yaw.kd;

}


// ============================================================
// READ PID INPUTS
// ============================================================

function readPIDInputs() {

    return {

        balance: {

            kp:
                Number(
                    pidFields.balanceKp.value
                ),

            ki:
                Number(
                    pidFields.balanceKi.value
                ),

            kd:
                Number(
                    pidFields.balanceKd.value
                )

        },


        speed: {

            kp:
                Number(
                    pidFields.speedKp.value
                ),

            ki:
                Number(
                    pidFields.speedKi.value
                ),

            kd:
                Number(
                    pidFields.speedKd.value
                )

        },


        yaw: {

            kp:
                Number(
                    pidFields.yawKp.value
                ),

            ki:
                Number(
                    pidFields.yawKi.value
                ),

            kd:
                Number(
                    pidFields.yawKd.value
                )

        }

    };

}


// ============================================================
// VALIDATE PID
// ============================================================

function validatePID(profile) {

    const values = [

        profile.balance.kp,
        profile.balance.ki,
        profile.balance.kd,

        profile.speed.kp,
        profile.speed.ki,
        profile.speed.kd,

        profile.yaw.kp,
        profile.yaw.ki,
        profile.yaw.kd

    ];


    return values.every(
        function (value) {

            return Number.isFinite(value)
                && value >= 0;

        }
    );

}


// ============================================================
// PID MESSAGE
// ============================================================

function setPIDMessage(
    message,
    type = ""
) {

    pidMessage.textContent =
        message;


    pidMessage.className =
        "pid-message";


    if (type) {

        pidMessage.classList.add(
            type
        );

    }

}


// ============================================================
// PROFILE SELECT
// ============================================================

profileSelect.addEventListener(
    "change",
    function () {

        loadProfile(
            profileSelect.value
        );

        setPIDMessage(
            `Loaded ${profileSelect.value}`
        );

    }
);


// ============================================================
// APPLY PID
// ============================================================

document
    .getElementById("apply-pid")
    .addEventListener(
        "click",
        function () {

            const pid =
                readPIDInputs();


            if (!validatePID(pid)) {

                setPIDMessage(
                    "Invalid PID values",
                    "error"
                );

                return;

            }


            sendJSON({

                type: "pid",

                profile:
                    profileSelect.value,

                balance:
                    pid.balance,

                speed:
                    pid.speed,

                yaw:
                    pid.yaw

            });


            setPIDMessage(
                "PID applied",
                "success"
            );

        }
    );


// ============================================================
// SAVE PROFILE
// ============================================================

document
    .getElementById("save-profile")
    .addEventListener(
        "click",
        function () {

            const name =
                profileSelect.value;


            const pid =
                readPIDInputs();


            if (!validatePID(pid)) {

                setPIDMessage(
                    "Invalid PID values",
                    "error"
                );

                return;

            }


            profiles[name] =
                pid;


            saveProfilesToStorage();


            setPIDMessage(
                `${name} saved`,
                "success"
            );

        }
    );


// ============================================================
// ACTIVATE PROFILE
// ============================================================

document
    .getElementById("activate-profile")
    .addEventListener(
        "click",
        function () {

            const name =
                profileSelect.value;


            const profile =
                profiles[name];


            if (!profile) {
                return;
            }


            activeProfile =
                name;


            localStorage.setItem(
                "singfast_active_profile",
                activeProfile
            );


            loadProfile(name);


            updateActiveProfileText();


            sendJSON({

                type: "pid_activate",

                profile: name,

                balance:
                    profile.balance,

                speed:
                    profile.speed,

                yaw:
                    profile.yaw

            });


            setPIDMessage(
                `${name} activated`,
                "success"
            );

        }
    );


// ============================================================
// NEW PROFILE
// ============================================================

document
    .getElementById("new-profile")
    .addEventListener(
        "click",
        function () {

            const name =
                prompt(
                    "Enter new PID profile name:"
                );


            if (!name) {
                return;
            }


            const trimmed =
                name.trim();


            if (!trimmed) {
                return;
            }


            if (profiles[trimmed]) {

                setPIDMessage(
                    "Profile already exists",
                    "error"
                );

                return;

            }


            profiles[trimmed] = {

                balance: {
                    kp: 25,
                    ki: 0.5,
                    kd: 0.8
                },

                speed: {
                    kp: 1,
                    ki: 0.1,
                    kd: 0.05
                },

                yaw: {
                    kp: 1,
                    ki: 0,
                    kd: 0.1
                }

            };


            saveProfilesToStorage();


            refreshProfileSelect();


            profileSelect.value =
                trimmed;


            loadProfile(trimmed);


            setPIDMessage(
                `${trimmed} created`,
                "success"
            );

        }
    );


// ============================================================
// DELETE PROFILE
// ============================================================

document
    .getElementById("delete-profile")
    .addEventListener(
        "click",
        function () {

            const name =
                profileSelect.value;


            if (
                name === "Default"
            ) {

                setPIDMessage(
                    "Default profile cannot be deleted",
                    "error"
                );

                return;

            }


            if (
                !confirm(
                    `Delete profile "${name}"?`
                )
            ) {

                return;

            }


            delete profiles[name];


            if (activeProfile === name) {

                activeProfile =
                    "Default";


                localStorage.setItem(
                    "singfast_active_profile",
                    activeProfile
                );

            }


            saveProfilesToStorage();


            refreshProfileSelect();


            loadProfile(
                profileSelect.value
            );


            setPIDMessage(
                `${name} deleted`,
                "success"
            );

        }
    );


// ============================================================
// INITIAL PID SETUP
// ============================================================

refreshProfileSelect();

profileSelect.value =
    profiles[activeProfile]
        ? activeProfile
        : "Default";

loadProfile(
    profileSelect.value
);

updateActiveProfileText();


// ============================================================
// INITIAL CONNECTION STATE
// ============================================================

setConnectionStatus(
    "CONNECTING"
);

setInternalLatency(
    Infinity
);

setSystemMessage(
    "Connecting...",
    "warning"
);


// ============================================================
// START CONNECTION
// ============================================================

connectESP32();