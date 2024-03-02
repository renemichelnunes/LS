
#ifndef SCRIPT_JS_H
#define SCRIPT_JS_H

const char* script_js = R"rawliteral(

    let ws = null;
    let contactID = "";
    let editContactID = "";
    let contactName = "";
    let meID = "111111";
    let dx = false;

    let contextMenu = document.querySelector('.contextMenu');
    let currentTarget = null;

    let x = 0;
    let y = 0;

    var notificationCounter = 0;
    var notificationButton = document.getElementById("notification-button");
    var notificationArea = document.getElementById("notification-area");
    var timeoutId;

    // Function to show the notification area
    function showNotificationArea() {
    notificationArea.classList.remove("hidden");
    notificationArea.focus();
    }

    // Function to hide the notification area
    function hideNotificationArea() {
    notificationArea.classList.add("hidden");
    }

    // Function to hide the notification area after 3 seconds
    function hideNotificationAreaAfterDelay() {
    timeoutId = setTimeout(function() {
        hideNotificationArea();
    }, 3000);
    }

    // Function to clear the timeout and reset it to 3 seconds
    function resetTimeout() {
    clearTimeout(timeoutId);
    hideNotificationAreaAfterDelay();
    }

    // Function to clear the notification list and reset the counter
    function clearNotifications() {
    document.getElementById("notification-list").innerHTML = "";
    notificationCounter = 0;
    document.getElementById("notification-counter").textContent = notificationCounter;
    }

    function scrollToBottom() {
        var notificationArea = document.getElementById("notification-area");
        notificationArea.scrollTop = notificationArea.scrollHeight;
    }

    // Function to show a notification
    function showNotification(title, message) {
        var notificationElement = document.createElement("div");
        notificationElement.classList.add("notification");

        var titleElement = document.createElement("div");
        titleElement.textContent = title;
        titleElement.classList.add("notification-title");

        var messageElement = document.createElement("div");
        messageElement.textContent = message;
        messageElement.classList.add("notification-message");

        notificationElement.appendChild(titleElement);
        notificationElement.appendChild(messageElement);

        var notificationList = document.getElementById("notification-list");
        notificationList.appendChild(notificationElement);

        // Increment the counter and update the button text
        notificationCounter++;
        document.getElementById("notification-counter").textContent = notificationCounter;

        // Show the notification area
        showNotificationArea();
        scrollToBottom();

        // Reset the timeout to hide the notification area
        resetTimeout();
    }

    // Event listener for the notification button
    notificationButton.addEventListener("click", function() {
    if (notificationArea.classList.contains("hidden")) {
        showNotificationArea();
    } else {
        hideNotificationArea();
    }
    });

    // Event listener for the notification area losing focus
    notificationArea.addEventListener("blur", hideNotificationAreaAfterDelay);

    // Event listener for the clear link
    document.getElementById("clear-link").addEventListener("click", clearNotifications);


    function saveConfig(){
        var name = document.getElementById('settings_name').value;
        var id = document.getElementById('settings_id').value;

        if(name === '' || id === ''){
            alert('Name or ID cannot be empty.');
            return;
        }
        console.log(JSON.stringify({"command" : "set_name_id", "name" : name, "id" : id}));
        if(ws !== null){
            ws.send(JSON.stringify({"command" : "set_name_id", "name" : name, "id" : id}));
        }
    }

    function setDateTime(){
        var date = document.getElementById('settings_date');
        var time = document.getElementById('settings_time');
        var dateTime = new Date(date.value);
        var d = dateTime.getDate() + 1;
        var m = dateTime.getMonth() + 1;
        var y = dateTime.getFullYear();
        var [hh, mm] = time.value.split(':');

        console.log(JSON.stringify({"command" : "set_date", "d" : d, "m" : m, "y" : y, "hh" : hh, "mm" : mm}));
        if(ws !== null){
            ws.send(JSON.stringify({"command" : "set_date", "d" : d, "m" : m, "y" : y, "hh" : hh, "mm" : mm}));
        }
    }

    function dxmode(){
        dx = true;
        console.log(JSON.stringify({"command" : "set_dx_mode", "dx" : dx}));
        if(ws !== null)
            ws.send(JSON.stringify({"command" : "set_dx_mode", "dx" : dx}));
    }

    function normalMode(){
        dx = false;
        console.log(JSON.stringify({"command" : "set_dx_mode", "dx" : dx}));
        if(ws !== null)
            ws.send(JSON.stringify({"command" : "set_dx_mode", "dx" : dx}));
    }

    function setDxToggle(){
        var button = document.getElementById('toggleBtn');
        button.classList.toggle('active');
    }

    function restrictInput(input){
        input.value = input.value.replace(/[^abcdefABCDEF0-9]/g, '');
    }

    function setUIColor(){
        var input = document.getElementById('settings_uicolor');
        if(ws !== null && input.value !== '')
            ws.send(JSON.stringify({"command" : "set_ui_color", "color" : input.value}));
        console.log(JSON.stringify({"command" : "set_ui_color", "color" : input.value}));
    }

    function generateID() {
        const alphanum = '0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz';
        let id = '';

        for (let i = 0; i < 6; ++i) {
            id += alphanum.charAt(Math.floor(Math.random() * alphanum.length));
        }
        return id;
    }

    function sendGeneratedID(){
        var input = document.getElementById('settings_id');
        let id = generateID();
        window.confirm("Advice your contacts about your new ID or they won't receive your messages. Cancel if you want to maintain your actual ID.");
        input.value = id;
    }


    function setBrightness(level) {
        var brightnessRange = document.getElementById('brightnessRange');
        var brightness_value = document.getElementById('brightness_value');

        // Perform actions based on the selected brightness level
        brightness_value.innerHTML = level;
        if(ws !== null)
            ws.send(JSON.stringify({"command":"set_brightness", "brightness":level}));
        console.log(JSON.stringify({"command":"set_brightness", "brightness":level}));
    }

    function showContextMenu(event, id, name) {
        event.preventDefault();
        currentTarget = event.currentTarget;
        contextMenu.style.display = 'block';
        contextMenu.style.left = event.clientX + 'px';
        contextMenu.style.top = event.clientY + 'px';
        contextMenu.setAttribute('data-id', id);
        contextMenu.setAttribute('data-name', name);
        console.log("id " + id + " " + name);
        document.addEventListener('mousedown', handleOutsideClick);
    }

    function handleOutsideClick(event) {
        if (!contextMenu.contains(event.target) && event.target !== currentTarget) {
            hideContextMenu();
        }
    }

    function hideContextMenu() {
        contextMenu.style.display = 'none';
        document.removeEventListener('mousedown', handleOutsideClick);
    }

    function edit(event) {
        event.stopPropagation();
        let id = contextMenu.getAttribute('data-id');
        let name = contextMenu.getAttribute('data-name');
        editContact(id, name);
        hideContextMenu();
    }

    function remove(event) {
        event.stopPropagation();
        let id = contextMenu.getAttribute('data-id');
        if(window.confirm('Delete contact?')){
            if(ws !== null){
                ws.send(JSON.stringify({"command" : "del_contact", "id" : id}));
            }
        }
        hideContextMenu();
    }

    document.getElementById('editContact').addEventListener('click', edit);
    document.getElementById('deleteContact').addEventListener('click', remove);

    document.addEventListener("DOMContentLoaded", function() {
        const tabs = document.querySelectorAll('.tab');
        const contents = document.querySelectorAll('.content');
        const nameListItems = document.querySelectorAll('#tab1 .name-list ul li');

        // Function to select a tab
        function selectTab(tabId) {
            const tab = document.querySelector(`[data-tab="${tabId}"]`);
            tab.classList.add('selected');

            // Show content of the selected tab and hide others
            contents.forEach(content => {
                if (content.id === tabId) {
                    content.style.display = 'block';
                } else {
                    content.style.display = 'none';
                }
            });
        }

        // Select the first tab as soon as the page loads
        selectTab('tab1');

        // Event listener for tab clicks
        tabs.forEach(tab => {
            tab.addEventListener('click', function() {
                const tabId = tab.getAttribute('data-tab');

                // Remove 'selected' class from all tabs
                tabs.forEach(otherTab => {
                    otherTab.classList.remove('selected');
                });

                // Select the clicked tab
                selectTab(tabId);
            });
        });
    });

    function changeStatus(id, status){
        var list = document.querySelector('.name-list ul');
        var count = list.childElementCount;

        for(i = 0; i < count; i++){
            if(list.children[i].children[0].value === id){
                if(status == true)
                    list.children[i].children[1].children[0].className = 'status-on';
                else
                    list.children[i].children[1].children[0].className = 'status';
                break;
            }
        }
    }

    function loadConstacts(contactList) {
        const nameList = document.querySelector('.name-list ul');
        nameList.innerHTML = "";
        let selectedListItem = null;

        contactList.contacts.forEach(function(c) {
            const name = c.name;
            const id = c.id;
            const status = c.status;

            const listItem = document.createElement('li');
            listItem.oncontextmenu = function(event){
                showContextMenu(event, id, name);
            };
            const listItemContent = document.createElement('div'); // Create a div for the content
            listItemContent.textContent = name;

            const hiddenInput = document.createElement('input');
            hiddenInput.type = 'hidden';
            hiddenInput.value = id;

            const statusDiv = document.createElement('div');
            statusDiv.id = 'status';
            if(status === 'on')
                statusDiv.className = 'status-on';
            else
                statusDiv.className = 'status';

            listItem.appendChild(hiddenInput);
            listItem.appendChild(listItemContent); // Append the div content to the li
            nameList.appendChild(listItem);

            listItemContent.appendChild(statusDiv);

            const last_msg = document.createElement('div');
            last_msg.textContent = 'is there a way using only css to limit a text to a fized size?';
            last_msg.classList = 'last_msg';
            listItem.appendChild(last_msg);

            // Add event listener to each list item
            listItem.addEventListener('click', function() {
                // Remove background color from previously selected item
                if (selectedListItem !== null) {
                    selectedListItem.style.backgroundColor = '';
                }

                // Change background color of the clicked item
                listItem.style.backgroundColor = '#ffe4c4'; // Change to your desired background color
                selectedListItem = listItem;

                const selectedName = name;
                const selectedId = id;
                contactID = id;
                contactName = name;
                ws.send(JSON.stringify({"command" : "sel_contact", "id" : contactID}));
                console.log("Selected name: " + selectedName + ", ID: " + selectedId);
            });
        });
    };

    // Function to add a new message to the chat and scroll to the last message
    function addMessage(message) {
        const textScroller = document.querySelector('.text-scroller');

        textScroller.append(message);
        textScroller.appendChild(document.createElement("br"));
        // Scroll to the last message
        textScroller.scrollTo(0, textScroller.scrollHeight);
    }

    function sendMesage(cid, message){
        const data = {
            command : 'send',
            id : cid,
            msg : message
        }
        if(ws !== null)
            ws.send(JSON.stringify(data));
    }

    document.addEventListener("DOMContentLoaded", function() {
        const textArea = document.querySelector('.input-textarea');
        textArea.addEventListener('keydown', function(event) {
            // Check if Enter key is pressed
            if (event.key === 'Enter' && !event.shiftKey) {
                event.preventDefault(); // Prevent new line in textarea
                const message = textArea.value.replace(/\r?\n|\r/g, '<br>'); // Get message content
                if (message !== '') {
                    var currentDate = new Date();
                    var formattedDate = formatDate(currentDate);
                    sendMesage(meID, message.replace(/<br>/g, '\n'));
                    add_contact_msg('Me', formattedDate, message);
                    textArea.value = ''; // Clear textarea
                }
            }
        });
    });

    document.addEventListener("DOMContentLoaded", function() {
        var settings_date = document.getElementById('settings_date');
        var settings_time = document.getElementById('settings_time');
        var currentDate = new Date();
        var hours = currentDate.getHours();
        var minutes = currentDate.getMinutes();
        var formattedTime = (hours < 10 ? '0' : '') + hours + ':' + (minutes < 10 ? '0' : '') + minutes;
        var brightnessRange = document.getElementById('brightnessRange');
        var brightness_value = document.getElementById('brightness_value');

        brightness_value.innerHTML = brightnessRange.value;

        settings_date.value = currentDate.toISOString().split('T')[0];
        settings_time.value = formattedTime;

        var toggleButton = document.getElementById('toggleBtn');
        toggleButton.addEventListener('click', function() {
            // Toggle the 'active' class on button click
            this.classList.toggle('active');
            if (this.classList.contains('active')) {
                dxmode();
            } else {
                normalMode();
            }
        });

        const colors = [
            "#000000", "#800000", "#008000", "#808000", "#000080", "#800080", "#008080", "#C0C0C0",
            "#808080", "#FF0000", "#00FF00", "#FFFF00", "#0000FF", "#FF00FF", "#00FFFF", "#FFFFFF",
            "#000000", "#00005F", "#000087", "#0000AF", "#0000D7", "#0000FF", "#005F00", "#005F5F",
            "#005F87", "#005FAF", "#005FD7", "#005FFF", "#008700", "#00875F", "#008787", "#0087AF",
            "#0087D7", "#0087FF", "#00AF00", "#00AF5F", "#00AF87", "#00AFAF", "#00AFD7", "#00AFFF",
            "#00D700", "#00D75F", "#00D787", "#00D7AF", "#00D7D7", "#00D7FF", "#00FF00", "#00FF5F",
            "#00FF87", "#00FFAF", "#00FFD7", "#00FFFF", "#5F0000", "#5F005F", "#5F0087", "#5F00AF",
            "#5F00D7", "#5F00FF", "#5F5F00", "#5F5F5F", "#5F5F87", "#5F5FAF", "#5F5FD7", "#5F5FFF",
            "#5F8700", "#5F875F", "#5F8787", "#5F87AF", "#5F87D7", "#5F87FF", "#5FAF00", "#5FAF5F",
            "#5FAF87", "#5FAFAF", "#5FAFD7", "#5FAFFF", "#5FD700", "#5FD75F", "#5FD787", "#5FD7AF",
            "#5FD7D7", "#5FD7FF", "#5FFF00", "#5FFF5F", "#5FFF87", "#5FFFAF", "#5FFFD7", "#5FFFFF",
            "#870000", "#87005F", "#870087", "#8700AF", "#8700D7", "#8700FF", "#875F00", "#875F5F",
            "#875F87", "#875FAF", "#875FD7", "#875FFF", "#878700", "#87875F", "#878787", "#8787AF",
            "#8787D7", "#8787FF", "#87AF00", "#87AF5F", "#87AF87", "#87AFAF", "#87AFD7", "#87AFFF",
            "#87D700", "#87D75F", "#87D787", "#87D7AF", "#87D7D7", "#87D7FF", "#87FF00", "#87FF5F",
            "#87FF87", "#87FFAF", "#87FFD7", "#87FFFF", "#AF0000", "#AF005F", "#AF0087", "#AF00AF",
            "#AF00D7", "#AF00FF", "#AF5F00", "#AF5F5F", "#AF5F87", "#AF5FAF", "#AF5FD7", "#AF5FFF",
            "#AF8700", "#AF875F", "#AF8787", "#AF87AF", "#AF87D7", "#AF87FF", "#AFAF00", "#AFAF5F",
            "#AFAF87", "#AFAFAF", "#AFAFD7", "#AFAFFF", "#AFD700", "#AFD75F", "#AFD787", "#AFD7AF",
            "#AFD7D7", "#AFD7FF", "#AFFF00", "#AFFF5F", "#AFFF87", "#AFFFAF", "#AFFFD7", "#AFFFFF",
            "#D70000", "#D7005F", "#D70087", "#D700AF", "#D700D7", "#D700FF", "#D75F00", "#D75F5F",
            "#D75F87", "#D75FAF", "#D75FD7", "#D75FFF", "#D78700", "#D7875F", "#D78787", "#D787AF",
            "#D787D7", "#D787FF", "#D7AF00", "#D7AF5F", "#D7AF87", "#D7AFAF", "#D7AFD7", "#D7AFFF",
            "#D7D700", "#D7D75F", "#D7D787", "#D7D7AF", "#D7D7D7", "#D7D7FF", "#D7FF00", "#D7FF5F",
            "#D7FF87", "#D7FFAF", "#D7FFD7", "#D7FFFF", "#FF0000", "#FF005F", "#FF0087", "#FF00AF",
            "#FF00D7", "#FF00FF", "#FF5F00", "#FF5F5F", "#FF5F87", "#FF5FAF", "#FF5FD7", "#FF5FFF",
            "#FF8700", "#FF875F", "#FF8787", "#FF87AF", "#FF87D7", "#FF87FF", "#FFAF00", "#FFAF5F",
            "#FFAF87", "#FFAFAF", "#FFAFD7", "#FFAFFF", "#FFD700", "#FFD75F", "#FFD787", "#FFD7AF",
            "#FFD7D7", "#FFD7FF", "#FFFF00", "#FFFF5F", "#FFFF87", "#FFFFAF", "#FFFFD7", "#FFFFFF",
            "#080808", "#121212", "#1C1C1C", "#262626", "#303030", "#3A3A3A", "#444444", "#4E4E4E",
            "#585858", "#606060", "#666666", "#767676", "#808080", "#8A8A8A", "#949494", "#9E9E9E",
            "#A8A8A8", "#B2B2B2", "#BCBCBC", "#C6C6C6", "#D0D0D0", "#DADADA", "#E4E4E4", "#EEEEEE"
        ];

        const colorPickerButton = document.getElementById('color-picker-button');
        const colorPicker = document.querySelector('.color-picker');
        colorPicker.style.display = 'none';

        colors.forEach(color => {
            const colorDiv = document.createElement('div');
            colorDiv.style.backgroundColor = color;
            colorDiv.addEventListener('click', () => selectColor(color));
            colorPicker.appendChild(colorDiv);
        });

        function selectColor(color) {
            var input = document.getElementById('settings_uicolor');
            // Apply the selected color to the button
            colorPickerButton.style.backgroundColor = color;
            input.value = color.replace(/#/g, '');
            // Hide the color picker
            colorPicker.style.display = 'none';
        }

        colorPickerButton.addEventListener('click', () => {
            // Toggle the display of the color picker
            if (colorPicker.style.display === 'none') {
                colorPicker.style.display = 'grid';
            } else {
                colorPicker.style.display = 'none';
            }
        });
        connect();
    });

    function formatDate(date) {
        // Get day, month, year, hours, and minutes
        var day = String(date.getDate()).padStart(2, '0');
        var month = String(date.getMonth() + 1).padStart(2, '0'); // Month starts from 0
        var year = date.getFullYear();
        var hours = String(date.getHours()).padStart(2, '0');
        var minutes = String(date.getMinutes()).padStart(2, '0');

        // Concatenate the parts to form the desired format
        return `${day}/${month}/${year} ${hours}:${minutes}`;
    }

    function isJSONObject(obj) {
        return typeof obj === 'object' && obj !== null && !Array.isArray(obj);
    }

    function add_contact_msg(name, msg_date, msg){
        var div_contact = document.createElement('div');
        div_contact.style.wordBreak = 'break-word';
        var textScroller = document.querySelector('.text-scroller')
        if(name !== "Me"){
            div_contact.className = 'contact_header';
        }
        else{
            div_contact.className = 'contact_header2'
        }
        div_contact.append(name + ' ' + msg_date);
        div_contact.appendChild(document.createElement('br'));
        div_contact.append(msg);
        textScroller.appendChild(div_contact);
        textScroller.scrollTo(0, textScroller.scrollHeight);
    }

    function parseData(data){
        try{
            let decData = JSON.parse(data);
            if(isJSONObject(decData)){
                if(decData.command === "contacts"){
                    loadConstacts(decData);
                }else if(decData.command === "msg_list"){
                    document.querySelector('.text-scroller').innerHTML = "";
                    decData.messages.forEach(function(m){
                        if(m.id === meID)
                            add_contact_msg("Me", m.msg_date, m.msg);
                        else
                            add_contact_msg(contactName, m.msg_date, m.msg);
                    });
                }else if(decData.command === "notification"){
                    alert(decData.message);
                }
            }else{
                addMessage(data);
            }
        }catch{
            addMessage(data);
        }
    }

    function clear_contacts_and_messages(){
        document.querySelector('.text-scroller').innerHTML = "";
        document.querySelector('.name-list ul').innerHTML = "";
    }

    function addContact(){
        console.log("add");
        ws = new WebSocket(location.protocol === 'https:' ? 'wss://' + window.location.host + ':9501' : 'ws://' + window.location.host + ':9501');
        ws.onopen = function(e){
            console.log(e);
        };
        ws.onerror = function(e){
            console.log(e);
        };
        ws.onmessage = function(e){
            parseData(e.data);
            console.log(e);
        };
        ws.onclose = function(e){
            clear_contacts_and_messages();
            console.log("Disconnected");
        }
    };



    function connect(){
        ws = new WebSocket(location.protocol === 'https:' ? 'wss://' + window.location.host + ':9501' : 'ws://' + window.location.host + ':9501');
        ws.onopen = function(e){
            console.log(e);
        };
        ws.onerror = function(e){
            console.log(e);
        };
        ws.onmessage = function(e){
            parseData(e.data);
            console.log(e);
        };
        ws.onclose = function(e){
            clear_contacts_and_messages();
            console.log("Disconnected");
        }
    };

    function delContact(){
        console.log('del');
        if(ws !== null){
            ws.close();
            ws = null;
            console.log('closed');
        }
    };

    function showNew() {
        document.getElementById("divNew").style.display = "block";
        document.getElementById("divNew").style.left = x + 'px';
        document.getElementById("divNew").style.top = y + 'px';
    }

    function hideNew() {
        document.getElementById("divNew").style.display = "none";
    }

    function confirmNew() {
        let id = document.getElementById("CID").value;
        let name = document.getElementById("CName").value;
        if(id === '' || name === ''){
            alert('ID or Name cannot be empty.');
            return;
        }
        if(ws !== null)
            ws.send(JSON.stringify({"command" : "new_contact",
                                    "id" : id,
                                    "name" : name}));
        hideNew(); // For demonstration, hiding modal after confirmation
    }

    function newContact(){
        showNew();
    }

    function showEdit(id, name){
        document.getElementById("divEdit").style.display = "block";
        document.getElementById("divEdit").style.left = x + 'px';
        document.getElementById("divEdit").style.top = y + 'px';
        document.getElementById("CIDedit").value = id;
        document.getElementById("CNameedit").value = name;
        editContactID = id;
    }

    function hideEdit(){
        document.getElementById("divEdit").style.display = "none";
    }

    function confirmEdit(){
        if(ws !== null){
            ws.send(JSON.stringify({"command" : "edit_contact",
                                    "id" : editContactID,
                                    "newid" : document.getElementById("CIDedit").value,
                                    "newname" : document.getElementById("CNameedit").value}));
        }
        editContactID = "";
        hideEdit();
    }

    function editContact(id, name){
        showEdit(id, name);
    }

    function getMouseCoordinates(event) {
        const x = event.clientX;
        const y = event.clientY;
        return { x: x, y: y };
    }

    document.addEventListener('mousemove', function(event) {
        const coordinates = getMouseCoordinates(event);
        x = coordinates.x;
        y = coordinates.y;
    });


)rawliteral";

#endif
