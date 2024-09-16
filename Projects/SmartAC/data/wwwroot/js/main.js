"use strict";
const url = 'http://10.0.0.79:8222/json'; // Replace <ESP32-IP> with your ESP32's IP address
class SmartAC {
    action(_action) {
        // JSON data to be sent in the request
        const data = {
            action: _action,
        };
        this.sendData(data);
    }
    sendData(data) {
        let startTime = performance.now();
        console.log("start");
        $.ajax({
            url: url,
            type: 'POST',
            contentType: 'application/json',
            data: JSON.stringify(data),
            success: function (jsonResponse) {
                let timeTaken = performance.now() - startTime;
                console.log(`end, ${timeTaken}`, jsonResponse);
            },
            error: function (error) {
                console.error('Error:', error);
            }
        });
    }
}
