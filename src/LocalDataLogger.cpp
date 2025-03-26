#include "LocalDataLogger.h"

bool LocalDataLogger::begin() {
	// Set description
	Description.actionQuantity = 1;
	Description.type = "datalogger";
	Description.name = "Local Data Logger";
	Description.actions = {{"Log data", 0}};
	bool result = false;
	if (!checkConfig(config_path)) {
		// Set defaults
		current_config = { .name = "LocalData.csv", .enabled = false };
		task_config = { .taskName = "LocalDataLogger", .taskPeriod = 10000 };
		path = "/data/" + current_config.name;
		result = saveConfig(config_path, getConfig());
	} else {
		// Load settings
		result = setConfig(Storage::readFile(config_path), false);
	}
	return result;
}

/// @brief Enables the local data logger
/// @param enable True to enable, false to disable 
/// @return True on success
bool LocalDataLogger::enableLogging(bool enable) {
	current_config.enabled = enable;
	return enableTask(enable);
}

/// @brief Receives an action
/// @param action The action to process (only option is 0 for set output)
/// @param payload A 0 or 1 to set the pin low or high
/// @return JSON response with OK
std::tuple<bool, String> LocalDataLogger::receiveAction(int action, String payload) {
	if (action == 0) {
		runTask(LONG_MAX);
	}	
	return { true, R"({"Response": "OK"})" };
}

/// @brief Sets the configuration for this device
/// @param config A JSON string of the configuration settings
/// @param save If the configuration should be saved to a file
/// @return True on success
bool LocalDataLogger::setConfig(String config, bool save) {
	// Allocate the JSON document
  	JsonDocument doc;
	// Deserialize file contents
	DeserializationError error = deserializeJson(doc, config);
	// Test if parsing succeeds.
	if (error) {
		Logger.print(F("Deserialization failed: "));
		Logger.println(error.f_str());
		return false;
	}
	// Assign loaded values
	current_config.name = doc["name"].as<String>();
	current_config.enabled = doc["enabled"].as<bool>();
	task_config.taskPeriod = doc["samplingPeriod"].as<long>();
	task_config.taskName = doc["taskName"].as<std::string>();
	path = "/data/" + current_config.name;
	enableLogging(current_config.enabled);
	if (save) {
		return saveConfig(config_path, getConfig());
	}
	return true;
}

/// @brief Logs current data from all sensors
/// @param elapsed The time in ms since this task was last called
void LocalDataLogger::runTask(ulong elapsed) {
	if (taskPeriodTriggered(elapsed)) {
		if (!createDataFile()) {
			return;
		}
		String data = TimeInterface::getFormattedTime("%m-%d-%Y %T");
		// Allocate the JSON document
		JsonDocument doc;
		// Deserialize sensor info
		DeserializationError error = deserializeJson(doc, SensorManager::getLastMeasurement());
		// Test if parsing succeeds.
		if (error) {
			Logger.print(F("Deserialization failed: "));
			Logger.println(error.f_str());
			return;
		}
		for (const auto& m : doc["measurements"].as<JsonArray>()) {
			data += "," + m["value"].as<String>();
		}
		data += '\n';
		if (Storage::freeSpace() > data.length()) {
			Storage::appendToFile(path, data);
		}
	}
}

/// @brief Creates the data file with header if needed
/// @return True on success or if file exists
bool LocalDataLogger::createDataFile() {
	if (!Storage::fileExists(path)) {
		// Create file header
		String header = "time";
		// Allocate the JSON document
		JsonDocument doc;
		// Deserialize sensor info
		DeserializationError error = deserializeJson(doc, SensorManager::getSensorInfo());			
		// Test if parsing succeeds.
		if (error) {
			Logger.print(F("Deserialization failed: "));
			Logger.println(error.f_str());
			return false;
		}
		for (const auto& s : doc["sensors"].as<JsonArray>()) {
			for (const auto& p : s["parameters"].as<JsonArray>()) {
				header += "," + p["name"].as<String>() + " (" + p["unit"].as<String>() + ")";
			}
		}
		header += '\n';
		if (!Storage::fileExists("/data")) {
			Storage::createDir("/data");
		}
		if (!Storage::writeFile(path, header)) {
			return false;
		}
	}
	return true;
}

/// @brief Gets the current config
/// @return A JSON string of the config
String LocalDataLogger::getConfig() {
	// Allocate the JSON document
	JsonDocument doc;
	// Assign current values
	doc["name"] = current_config.name;
	doc["enabled"] = current_config.enabled;
	doc["samplingPeriod"] = task_config.taskPeriod;
	doc["taskName"] = task_config.taskName;

	// Create string to hold output
	String output;
	// Serialize to string
	serializeJson(doc, output);
	return output;
}