#include "LocalDataLogger.h"

/// @brief 
/// @param Name The device name 
/// @param configFile The name of the config file to use
LocalDataLogger::LocalDataLogger(String Name, String configFile) : Actor(Name) {
	config_path = "/settings/act/" + configFile;
}

bool LocalDataLogger::begin() {
	// Set description
	Description.type = "datalogger";
	Description.actions = {{"Log data", 0}};
	bool result = false;
	if (!checkConfig(config_path)) {
		// Set defaults
		task_config.set_taskName(Description.name.c_str());
		task_config.taskPeriod = 10000;
		path = "/data/" + current_config.fileName;
		result = saveConfig(config_path, getConfig());
	} else {
		// Load settings
		result = setConfig(Storage::readFile(config_path), false);
	}
	return result;
}

/// @brief Receives an action
/// @param action The action to process (only option is 0 for log data)
/// @param payload Not used
/// @return JSON response with success boolean
std::tuple<bool, String> LocalDataLogger::receiveAction(int action, String payload) {
	if (action == 0) {
		runTask(LONG_MAX);
	}	
	return { true, R"({"success": true})" };
}

/// @brief Logs current data from all sensors
/// @param elapsed The time in ms since this task was last called
void LocalDataLogger::runTask(ulong elapsed) {
	if (taskPeriodTriggered(elapsed)) {
		if (!createDataFile()) {
			return;
		}
		String data = TimeInterface::getFormattedTime(current_config.dateFormat);
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
	doc["Name"] = Description.name;
	doc["fileName"] = current_config.fileName;
	doc["enabled"] = current_config.enabled;
	doc["samplingPeriod"] = task_config.taskPeriod;
	doc["dateForamt"] = current_config.dateFormat;

	// Create string to hold output
	String output;
	// Serialize to string
	serializeJson(doc, output);
	return output;
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
	Description.name = doc["Name"].as<String>();
	current_config.fileName = doc["fileName"].as<String>();
	current_config.enabled = doc["enabled"].as<bool>();
	current_config.dateFormat = doc["dateFormat"].as<String>();
	task_config.taskPeriod = doc["samplingPeriod"].as<long>();
	task_config.set_taskName(Description.name.c_str());
	path = "/data/" + current_config.fileName;
	if (!enableTask(current_config.enabled)) {
		return false;
	}
	if (save) {
		return saveConfig(config_path, getConfig());
	}
	return true;
}