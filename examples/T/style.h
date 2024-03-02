#ifndef STYLE_CSS_H
#define STYLE_CSS_H

const char* style_css PROGMEM = R"rawliteral(

    body, html {
        margin: 0;
        padding: 0;
    }

    .container {
        display: flex;
        height: 100vh;
    }

    .title {
        background-color: #333;
        color: #fff;
        padding: 10px;
        text-align: center;
    }

    .tabs {
        display: flex;
        flex-direction: column;
        background-color: #f0f0f0;
    }

    .tab {
        padding: 10px;
        cursor: pointer;
        border-bottom: 1px solid #ccc;
    }

    .tab:hover {
        background-color: #dde;
    }

    .content {
        flex-grow: 1;
        padding: 20px;
        background-color: #ddd;
    }

    .selected {
        background-color: #ddd;
    }

    .name-list {
        height: 200px;
        width: 340px;
        overflow-y: scroll;
        background-color: #ddd;
    }

    .name-list ul {
        list-style-type: none;
        padding: 0;
        margin: 0;
    }

    .name-list ul li {
        margin-left: 10px;
        height: 50px;
        width: 300px;
        border-bottom: 1px solid #bbb;
        /*transition: transform 0.1s ease-in-out;*/
    }

    .name-list ul li:hover {
        background-color: #ffe4c4;
        /*transform: scale(1.05);*/
    }

    .name-list ul li:last-child {
        border-bottom: none;
    }

    .last_msg {
        font-size: small;
        white-space: nowrap;
        overflow: hidden;
        text-overflow: ellipsis;
        color: #333;
    }

    .chat-messages{
        overflow-y: auto;
        margin-top: -240px;
        margin-left: 340px;
        width: 340px;
        height: 300px;
    }
    .text-scroller {
        margin-left: 10px;
        margin-top: 7px;
        width: 320px;
        height: 200px;
        border: 0px solid black;
        overflow-block: auto;
        background-color: #ddd;
    }

    .input-textarea{
        margin-top: 10px;
        margin-left: 10px;
        width: 315px;
        background-color: #ddd;
        border-style: hidden;
    }

    .status{
        width: 20px;
        height: 20px;
        background-color: #bbb;
        margin-left: 280px;
        margin-top: -20px;
    }

    .status-on{
        width: 20px;
        height: 20px;
        background-color: #0f0;
        margin-left: 280px;
        margin-top: -20px;
    }

    .btn-add {
        margin-top: 10px;
    }

    .btn-add input {
        border-radius: 0;
        border-style: hidden;
    }

    .contact_header {
        margin-top: 5px;
        margin-left: 10px;
        margin-bottom: 5px;
        width: 300px;
        background-color: #ffe4c4;
        box-shadow: 0px 0px 10px 0px rgba(0, 0, 0, 1);
    }

    .contact_header2 {
        margin-top: 5px;
        margin-left: 10px;
        margin-bottom: 5px;
        width: 300px;
        background-color: #bed5de;
        box-shadow: 0px 0px 10px 0px rgba(0, 0, 0, 1);
    }

    #divNew {
        display: none;
        position: fixed;
        z-index: 1;
        left: 50%;
        top: 50%;
        transform: translate(-50%, -50%);
        width: 150px;
        height: 90px;
        background-color: #f0f0f0;
        border: 1px solid #ccc;
        padding: 20px;
    }
    #divNew textarea, #divNew button {
        display: block;
        margin-bottom: 10px;
    }

    #divNew button {
        border-radius: 0;
        border-style: hidden;
    }

    #divEdit {
        display: none;
        position: fixed;
        z-index: 1;
        left: 50%;
        top: 50%;
        transform: translate(-50%, -50%);
        width: 150px;
        height: 90px;
        background-color: #f0f0f0;
        border: 1px solid #ccc;
        padding: 20px;
    }
    #divEdit textarea, #divEdit button {
        display: block;
        margin-bottom: 10px;
    }

    #divEdit button {
        border-radius: 0;
        border-style: hidden;
    }

    .contextMenu {
        display: none;
        position: fixed;
        background-color: #f0f0f0;
        border: 1px solid #ccc;
        padding: 5px;
    }

    #editContact:hover {
        background-color: #ffe4c4;
    }

    #deleteContact:hover {
        background-color: #ffe4c4;
    }

    .settings_id {
        padding: 5px;
        width: 155px;
        height: 75px;
        border: solid 1px #b5b5b5;
    }

    #settings_name {
        width: 145px;
        border-style: hidden;
    }

    #settings_id {
        width: 50px;
        border-style: hidden;
    }

    #btngenerate {
        border-radius: 0;
        border-style: hidden;
    }

    .settings_date {
        margin-top: 5px;
        padding: 5px;
        width: 155px;
        height: 75px;
        border: solid 1px #b5b5b5;
    }

    .settings_date input {
        border-style: hidden;
    }

    .settings_dx {
        margin-top: -179px;
        margin-left: 172px;
        padding: 5px;
        width: 155px;
        height: 75px;
        border: solid 1px #b5b5b5;
    }

    .toggle-btn {
        display: inline-block;
        width: 60px;
        height: 30px;
        background-color: #ccc;
        border-radius: 15px;
        cursor: pointer;
        position: relative;
        border: 0px;
    }
    /* Style for the inner circle representing the toggle state */
    .toggle-btn .inner-circle {
        width: 24px;
        height: 24px;
        background-color: #808080;
        border-radius: 50%;
        position: absolute;
        top: 3px;
        left: 3px;
        transition: transform 0.3s ease;
        background-color: rgb(129, 255, 133);
        box-shadow: 0px 0px 10px 5px rgb(105, 255, 78);
    }
    /* Style for the active state */
    .toggle-btn.active .inner-circle {
        transform: translateX(25px);
        background-color: rgb(255, 189, 189);
        box-shadow: 0px 0px 10px 5px rgb(255, 135, 135);
    }

    .settings_ui {
        margin-top: 5px;
        margin-left: 172px;
        padding: 5px;
        width: 155px;
        height: 75px;
        border: solid 1px #b5b5b5;
    }

    .settings_ui input {
        border-style: hidden;
    }

    #settings_uicolor {
        width: 50px;
    }

    .settings_brightness {
        margin-top: 5px;
        padding: 5px;
        width: 155px;
        height: 75px;
        border: solid 1px #b5b5b5;
    }

    #brightness_value {
        width: 10px;
        height: 10px;
    }

    /* Styles for the brightness control */
    .brightness-control {
        width: 150px;
        height: 20px; /* Increased height for better usability */
        background-color: #f0f0f0;
        border-radius: 20px; /* Adjusted to match the height */
        position: relative;
        overflow: hidden;
    }

    input[type="range"] {
        -webkit-appearance: none;
        width: calc(100% - 5px); /* Adjusted width to accommodate the knob */
        height: 20px;
        background-color: transparent;
        outline: none;
        position: absolute;
        top: 50%;
        left: 0px; /* Positioned to align with the knob */
        transform: translateY(-50%);
        overflow: hidden;
    }

    input[type="range"]::-webkit-slider-thumb {
        -webkit-appearance: none;
        width: 20px; /* Increased width for better usability */
        height: 20px; /* Increased height for better usability */
        background-color: #000000;
        border-radius: 50%;
        cursor: grab;
        margin-top: -10px; /* Adjusted to center the knob vertically */
        margin-left: -20px; /* Adjusted to center the knob horizontally */
        padding: 2px; /* Increase the hit area */
    }

    .color-picker {
        display: none; /* Initially hidden */
        position: absolute;
        top: 50px; /* Adjust as needed */
        left: 50px; /* Adjust as needed */
        background-color: white;
        border: 1px solid #ccc;
        padding: 10px;
        z-index: 9999;
        display: grid;
        grid-template-columns: repeat(16, 1fr); /* Adjust as needed */
        gap: 5px;
    }
    .color-picker div {
        width: 20px;
        height: 20px;
        cursor: pointer;
    }

    .input_save {
        border-style: hidden;
    }

    #notification-button {
        position: fixed;
        right: 10px;
        top: 10px;
        padding: 5px 10px;
        background-color: #676767;
        color: #fff;
        border: none;
        border-radius: 0px;
        cursor: pointer;
    }

    #notification-counter {
        margin-left: 5px;
        background-color: #fff;
        color: #000000;
        padding: 0px 5px;
        border-radius: 50%;
    }

    #notification-area {
        position: fixed;
        top: 50px;
        right: 0px; /* Initially hidden */
        width: 300px;
        max-height: 80%;
        overflow-y: auto;
        background-color: #ddd;
        border: 5px solid #ddd;
        border-radius: 0px;
        padding: 2px;
        box-shadow: 5px 5px 5px rgba(0, 0, 0, 0.2);
        z-index: 1000;
    }

    .notification {
        margin-bottom: 5px; /* Separation between notifications */
        padding: 1px;
        background-color: #333; /* Background color of the notification */
        color: white; /* Text color of the notification */
        text-align: left;
        box-shadow: 5px 5px 5px rgba(0, 0, 0, 0.3);
    }

    .notification-title {
        color: white; /* Text color of the title */
        font-weight: bold;
        margin-bottom: 5px; /* Spacing between title and message */
        text-align: center;
    }

    .notification-message {
        color: black; /* Text color of the message */
        background-color: white; /* Background color of the message */
        white-space: pre-wrap;
    }

    .hidden {
        display: none;
    }

)rawliteral";

#endif
