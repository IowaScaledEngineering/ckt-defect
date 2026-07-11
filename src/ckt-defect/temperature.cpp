#include "temperature.h"

TemperatureManager::TemperatureManager(DetectorConfiguration* cfg) : _config(cfg) {
	// Register this instance globally if one doesn't exist yet
	if (_instance == nullptr)
	{
		_instance = this;
	}
}

TemperatureManager::~TemperatureManager() {
	if (_instance == this) {
		_instance = nullptr;
	}
}

TemperatureManager* TemperatureManager::getInstance() {
	return _instance;
}

bool TemperatureManager::begin() {
	_isInitialized = _sensor.begin();

	// Establish an initial target and prime the current temperature directly to it
	if (_config) {
		_targetTemperatureC = generateRandomTargetCelsius();
		_currentTemperatureC = _targetTemperatureC;
	}

	update(); // Run an update pass immediately
	return _isInitialized;
}

void TemperatureManager::update() {
	if (!_isInitialized || !_config || !_config->temperatureEnable) return;

	if (_config->temperatureReal) {
		// Mode: Real Chip Reading
		_currentTemperatureC = _sensor.readTemperatureC();
	} else {
		// Mode: Filtered Simulation Reading
		_updateCounter++;

		if (_updateCounter >= _targetInterval)
		{
			_targetTemperatureC = generateRandomTargetCelsius();
			_updateCounter = 0;
		}

		// Compute the delta and apply the fraction back to the current temperature
		float delta = _targetTemperatureC - _currentTemperatureC;
		_currentTemperatureC += (_filterCoefficient * delta);
	}
}

// Private helper to centralize absolute random generation logic
float TemperatureManager::generateRandomTargetCelsius() {
	if (!_config) return 0.0f;

	// Boundaries are now guaranteed to be natively in Celsius
	float minT = _config->minTemperatureC;
	float maxT = _config->maxTemperatureC;
	
	// Generate a smooth random number cleanly between the Celsius boundaries
	float randFraction = static_cast<float>(random(0, 10000)) / 10000.0f;
	return minT + randFraction * (maxT - minT);
}

void TemperatureManager::setFilterParameters(float n, uint32_t m) {
	if (n > 0.0f && n <= 1.0f) _filterCoefficient = n;
	if (m > 0) _targetInterval = m;
}

float TemperatureManager::getTemperatureC() const {
	return _currentTemperatureC;
}

float TemperatureManager::getTemperatureF() const {
	return (_currentTemperatureC * 1.8f) + 32.0f;
}

// Automatically formats output based on configuration's unit flag
float TemperatureManager::getTemperature() const
{
	if (_config && _config->temperatureUnitsF) {
		return getTemperatureF();
	}
	return _currentTemperatureC;
}
