"use strict";
class SmartAC {
    hello() {
        const url = '/json'; // Replace <ESP32-IP> with your ESP32's IP address
        // JSON data to be sent in the request
        const data = {
            name: "John",
            age: 30,
            hitSwitch: true,
        };
        console.log("hello1");
        $.ajax({
            url: url,
            type: 'POST',
            contentType: 'application/json',
            data: JSON.stringify(data),
            success: function (jsonResponse) {
                console.log("hello2");
                console.log('Response:', jsonResponse);
            },
            error: function (error) {
                console.error('Error:', error);
            }
        });
    }
}
