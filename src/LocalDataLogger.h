/*
* This file and associated .cpp file are licensed under the GPLv3 License Copyright (c) 2024 Sam Groveman
* 
* External libraries used:
* ArduinoJSON: https://arduinojson.org/
* 
* Contributors: Sam Groveman
*/

#pragma once
#include <Arduino.h>
#include <Actor.h>
#include <PeriodicTask.h>
#include <SensorManager.h>
#include <TimeInterface.h>
#include <Storage.h>
#include <ArduinoJson.h>

/// @brief Logs sensor data locally
class LocalDataLogger : public Actor, public PeriodicTask {
	protected:
		/// @brief Holds data logger configuration
		struct {
			/// @brief The file name and used to log data in data directory
			String name;

			/// @brief Enable data logging
			bool enabled;
		} current_config;

		/// @brief Full path to data file
		String path;

		/// @brief Path to configuration file
		const String config_path = "/settings/act/LocalLogger.json";
		bool enableLogging(bool enable);
		bool createDataFile();
		std::tuple<bool, String> receiveAction(int action, String payload);

	public:
		bool begin();
		String getConfig();
		bool setConfig(String config, bool save);
		void runTask(long elapsed);	
};