void dhtInit(sensor_t *sensor, uint32_t *dlayMS)
{
  dht.begin();
  dht.temperature().getSensor(sensor);
  dht.humidity().getSensor(sensor);
  *dlayMS = sensor->min_delay / 1000;
}

String getSensorData(DHT_Unified dth)
{
  JSONVar jsonDocument;
  jsonDocument["serial_number"] = SN;

  sensors_event_t event;
  dht.temperature().getEvent(&event);
  if (isnan(event.temperature)) {
    Serial.println(F("Error reading temperature!"));
    jsonDocument["errors"]["temperature"] = true;
    dataChange = true;
  }
  else {
    if (lastTemperature != event.temperature) {
      dataChange = true;
    }
    Serial.print("Last Temperature ");
    Serial.println(lastTemperature);
    jsonDocument["temperature"] = event.temperature;
    lastTemperature = event.temperature;
    Serial.print("Temperature ");
    Serial.println(event.temperature);
  }
  // Get humidity event and print its value.
  dht.humidity().getEvent(&event);
  if (isnan(event.relative_humidity)) {
    jsonDocument["errors"]["humidity"] = true;
    dataChange = true;
  }
  else {
    Serial.print("Last humidity ");
    Serial.println(lastHumidity);
    jsonDocument["humidity"] = event.relative_humidity;
    Serial.print("humidity ");
    Serial.println(event.relative_humidity);
    if (lastHumidity != event.relative_humidity) {
      dataChange = true;
    }
    lastHumidity = event.relative_humidity;
  }

  return JSON.stringify(jsonDocument);

}
