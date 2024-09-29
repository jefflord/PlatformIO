const url = 'http://10.0.0.79:8222/json'; // Replace <ESP32-IP> with your ESP32's IP address

class SmartAC {

    action(_action: string) {

        // JSON data to be sent in the request
        const data = {
            action: _action,
        };

        this.sendData(data);


    }

    private sendData(data: any) {
        let startTime = performance.now();
        console.log("start");
        $.ajax({
            url: url,
            type: 'POST',
            contentType: 'application/json',
            data: JSON.stringify(data),
            success: function (jsonResponse) {


                jsonResponse["minutes"] = (jsonResponse["millis"]/1000/60.0).toFixed(1);

                $("#responseData").text(JSON.stringify(jsonResponse, null, 5));
                let timeTaken = performance.now() - startTime;
                console.log(`end, ${timeTaken}`, jsonResponse);
            },
            error: function (error) {
                console.error('Error:', error);
            }
        });
    }
}
