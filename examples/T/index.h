#ifndef INDEX_HTML_H
#define INDEX_HTML_H

const char* index_html PROGMEM = R"rawliteral(

<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>T-Deck</title>
    <link rel="stylesheet" href="style.css">
    <div class="title">
        <h1>T-Deck</h1>
        <button id="notification-button">Notifications<span id="notification-counter">0</span></button>
        <div id="notification-area" class="hidden" tabindex="0">
        <div id="notification-list"></div>
        <a href="#" id="clear-link">Clear</a>
        </div>
    </div>
</head>
<body>
    <div class="contextMenu">
        <div id="editContact">Edit</div>
        <div id="deleteContact">Delete</div>
    </div>
    <div class="container">
        <div class="tabs">
            <div class="tab" data-tab="tab1">LoRa Chat</div>
            <div class="tab" data-tab="tab2">Status</div>
            <div class="tab" data-tab="tab3">Settings</div>
        </div>
        <div class="content" id="tab1">
            <div class="name-list">
                <ul>

                </ul>
            </div>
            <div class="btn-add">
                <input type="button" id="btnconnect" value="Connect" onclick="addContact()">
                <input type="button" id="btndisconnect" value="Disconnect" onclick="delContact()">
                <input type="button" id="btnnew" value="New" onclick="newContact()">
            </div>
            <div class="chat-messages">
                <div class="text-scroller">

                </div>
                <textarea id="msg-area" class="input-textarea" rows="4" placeholder="Type your message here, use 'Enter' to send and 'Shift + Enter' to add a new line..."></textarea>
            </div>
        </div>
        <div class="content" id="tab2" style="display: none;">
            Content of Tab 2
        </div>
        <div class="content" id="tab3" style="display: none;">
            <div class="settings_id">
                My ID
                <input id="settings_name" placeholder="Name">
                <br>
                <input id="settings_id" placeholder="ID" maxlength="6">
                <input type="button" id="btngenerate" value="Generate" onclick="sendGeneratedID()">
            </div>
            <div class="settings_date">
                Date
                <input type="date" id="settings_date">
                <br>
                <input type="time" id="settings_time" min="00:00" max="23:59" required>
                <input type="button" id="btnSetDate" value="Set" onclick="setDateTime()">
            </div>
            <div class="settings_dx">
                Normal
                <button class="toggle-btn" id="toggleBtn">
                    <div class="inner-circle"></div>
                </button>
                DX
            </div>
            <div class="settings_wifi">

            </div>
            <div class="settings_ui">
                UI Color
                <br>
                <input type="button" id="color-picker-button" value="Choose a color">
                <div class="color-picker"></div>
                <input id="settings_uicolor" maxlength="6" oninput="restrictInput(this)">
                <input type="button" value="Set" onclick="setUIColor()">
            </div>
            <div class="settings_brightness">
                Screen brightness
                <br>
                <div class="brightness-control">
                    <!-- Input range slider -->
                    <input type="range" min="1" max="10" value="5" step="1" oninput="setBrightness(this.value)" id="brightnessRange">
                    <br>
                </div>
                <div id="brightness_value"></div>
            </div>
            <input type="button" id="btn_save" class="input_save" value="Save" onclick="saveConfig()">
        </div>
    </div>
    <div id="divNew">
        <form id="frmNew">
            <textarea id="CID" rows="1" placeholder="ID"></textarea>
            <textarea id="CName" rows="1" placeholder="Name"></textarea>
            <input type="button" value="Confirm" onclick="confirmNew()">
            <input type="button" value="Close" onclick="hideNew()">
        </form>
    </div>
    <div id="divEdit">
        <form id="frmEdit">
            <textarea id="CIDedit" rows="1" placeholder="ID"></textarea>
            <textarea id="CNameedit" rows="1" placeholder="Name"></textarea>
            <input type="button" value="Confirm" onclick="confirmEdit()">
            <input type="button" value="Close" onclick="hideEdit()">
        </form>
    </div>
    <script src="script.js"></script>
</body>
</html>


)rawliteral";

#endif
