// HTML content
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>Smart Control Dashboard</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: Arial; text-align: center; margin: 0; padding: 0; }
    h1 { color: #2f4468; }
    p { font-size: 1.5rem; }
    .warning { color: red; font-weight: bold; }
    .safe { color: green; font-weight: bold; }
    .info { color: blue; font-weight: bold; }
    .topnav { overflow: hidden; background-color: #2f4468; color: white; font-size: 1.7rem; }
    .content { padding: 20px; }
    .card { background-color: white; box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5); padding: 20px; border-radius: 5px; }
    .cards { max-width: 900px; margin: 0 auto; display: grid; grid-gap: 2rem; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); }
    .reading { font-size: 2.8rem; }
    .packet { color: #bebebe; }
    .card.header { background-color: #fd7e14; color: white; }
    .card.header h4 { margin: 0; }
  </style>
</head>
<body>
  <div class="topnav">
    <h3>Smart Control Dashboard</h3>
  </div>
  <div class="content">
    <div class="cards">
    <div class="card message">
        <p id="lights_status">Loading...</p>
      </div>
    <div class="card message">
        <h2><strong>RFID Sensor:</strong></h2>
        <p id="rfid_status">Loading...</p>
      </div>
      <div class="card message">
        <h2><strong>Smoke Sensor:</strong></h2>
        <p id="smoke_status">Loading...</p>
      </div>
      <div class="card message">
        <h2><strong>Ultrasonic Sensor:</strong></h2>
        <p id="ultrasonic_status">Loading...</p>
      </div>
    </div>

  </div>

  <script>
    setInterval(() => {
      // Fetch smoke sensor data
      fetch('/smoke').then(response => response.text()).then(data => {
        document.getElementById('smoke_status').innerHTML = data;
      });

      // Fetch ultrasonic sensor data
      fetch('/ultrasonic').then(response => response.text()).then(data => {
        document.getElementById('ultrasonic_status').innerHTML = data;
      });
      fetch('/rfid').then(response => response.text()).then(data => {
        document.getElementById('rfid_status').innerHTML = data;
      });
      fetch('/lights').then(response => response.text()).then(data => {
        document.getElementById('lights_status').innerHTML = data;
      });
    
    }, 500);  // Poll every 500ms for faster updates

  </script>
</body>
</html>
)rawliteral";
