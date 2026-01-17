const functions = require('@google-cloud/functions-framework');
const { Firestore } = require('@google-cloud/firestore');

// Initialize Firestore
const firestore = new Firestore();

functions.http('receiveAirData', async (req, res) => {
  // 1. Basic CORS headers to allow connections (good practice)
  res.set('Access-Control-Allow-Origin', '*');

  if (req.method === 'OPTIONS') {
    // Send response to OPTIONS requests
    res.set('Access-Control-Allow-Methods', 'POST');
    res.set('Access-Control-Allow-Headers', 'Content-Type');
    res.set('Access-Control-Max-Age', '3600');
    res.status(204).send('');
    return;
  }

  // 2. Parse the incoming JSON data from ESP32
  const data = req.body;
  
  // Check if gas_value exists in the sent data
  if (data && data.hasOwnProperty('gas_value')) {
    const gasValue = data.gas_value;
    
    // Determine status
    let status = "Normal";
    if (gasValue > 1800) {
        status = "WARNING";
    }

    try {
      // 3. Write to Firestore Collection "sensor_readings"
      // Note: This creates the collection automatically if it doesn't exist
      await firestore.collection('sensor_readings').add({
        gas_value: gasValue,
        status: status,
        timestamp: Firestore.FieldValue.serverTimestamp()
      });

      console.log(`Stored gas value: ${gasValue}`);
      res.status(200).send(`Success: ${gasValue}`);
      
    } catch (error) {
      console.error("Error writing to Firestore:", error);
      res.status(500).send("Internal Server Error");
    }
  } else {
    res.status(400).send("Bad Request: Missing gas_value");
  }
});