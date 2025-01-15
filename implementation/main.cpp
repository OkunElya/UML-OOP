// main.cpp
#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <chrono>
#include <thread>
#include <algorithm>

// Enums
enum class ConnectionType { WiFi, Zigbee, ZWave, Bluetooth, Ethernet };
enum class PowerSource { Battery, Mains, Solar, Other };
enum class EventType { DEVICE_EVENT, APP_EVENT };
enum class Direction { TO_APP, FROM_APP };

// Forward declarations
class MainNode;
class Event;
class AppEvent;
class DeviceEvent;

// Base Event class
class Event {
protected:
    EventType type;
    int deviceId;
    long long timestamp;

public:
    Event(EventType t, int id) : type(t), deviceId(id) {
        timestamp = std::chrono::system_clock::now().time_since_epoch().count();
    }
    virtual ~Event() = default;

    EventType getType() const { return type; }
    int getDeviceId() const { return deviceId; }
    virtual std::string toJson() const {
        return "{ type: " + std::to_string(static_cast<int>(type)) +
            ", deviceId: " + std::to_string(deviceId) + " }";
    }
};

class DeviceEvent : public Event {
    std::string eventData;
public:
    DeviceEvent(int deviceId, std::string data)
        : Event(EventType::DEVICE_EVENT, deviceId), eventData(data) {
        std::cout << "DeviceEvent created: " << toJson() << std::endl;
    }

    std::string toJson() const override {
        return "{ type: DEVICE_EVENT, deviceId: " + std::to_string(deviceId) +
            ", data: " + eventData + " }";
    }
};

class AppEvent : public Event {
    std::string data;
    Direction direction;
public:
    AppEvent(int deviceId, std::string d, Direction dir)
        : Event(EventType::APP_EVENT, deviceId), data(d), direction(dir) {
        std::cout << "AppEvent created: " << toJson() << std::endl;
    }

    Direction getDirection() const { return direction; }
    std::string getData() const { return data; }

    std::string toJson() const override {
        return "{ type: APP_EVENT, deviceId: " + std::to_string(deviceId) +
            ", direction: " + (direction == Direction::TO_APP ? "TO_APP" : "FROM_APP") +
            ", data: " + data + " }";
    }
};

// Smart Device classes
class SmartDevice {
protected:
    int deviceId;
    std::string name;
    std::vector<ConnectionType> connectionTypes;
    std::vector<PowerSource> powerSources;
    MainNode* mainNode;

public:
    SmartDevice(int id, std::string n) : deviceId(id), name(n), mainNode(nullptr) {
        std::cout << "Created SmartDevice: " << name << " (ID: " << deviceId << ")" << std::endl;
    }

    virtual ~SmartDevice() = default;

    void setMainNode(MainNode* node) { mainNode = node; }
    int getId() const { return deviceId; }
    std::string getName() const { return name; }

    virtual void handleEvent(const Event& event) = 0;
    void sendEvent(const Event& event);  // Implementation after MainNode definition
};

class SmartLight : public SmartDevice {//лампа от китайских друзей не выдержала и выгорела.. теперь светит только оттенками серого
    bool isOn;
    int brightness;

public:
    SmartLight(int id, std::string name)
        : SmartDevice(id, name), isOn(false), brightness(0) {
        connectionTypes = { ConnectionType::WiFi };
        powerSources = { PowerSource::Mains };
    }

    void handleEvent(const Event& event) override {
        if (event.getType() == EventType::APP_EVENT) {
            const AppEvent& appEvent = static_cast<const AppEvent&>(event);
            if (appEvent.getData() == "TOGGLE") {
                isOn = !isOn;
                std::cout << "Light " << name << " turned " << (isOn ? "ON" : "OFF") << std::endl;

                // Send feedback
                sendEvent(AppEvent(deviceId,
                    "Light state changed to " + std::string(isOn ? "ON" : "OFF"),
                    Direction::TO_APP));
            }
        }
    }
};

class SmartThermostat : public SmartDevice {
    float temperature;
    float targetTemp;

public:
    SmartThermostat(int id, std::string name)
        : SmartDevice(id, name), temperature(20.0f), targetTemp(22.0f) {
        connectionTypes = { ConnectionType::WiFi, ConnectionType::ZWave };
        powerSources = { PowerSource::Mains };
    }

    void handleEvent(const Event& event) override {
        if (event.getType() == EventType::APP_EVENT) {
            const AppEvent& appEvent = static_cast<const AppEvent&>(event);
            if (appEvent.getData().find("SET_TEMP:") == 0) {
                targetTemp = std::stof(appEvent.getData().substr(9));
                std::cout << "Thermostat " << name << " target temperature set to "
                    << targetTemp << "C" << std::endl;

                sendEvent(AppEvent(deviceId,
                    "Target temperature set to " + std::to_string(targetTemp),
                    Direction::TO_APP));
            }
        }
    }

    void simulateTemperatureChange(float newTemp) {
        temperature = newTemp;
        std::cout << "Temperature changed to " << temperature << "°C" << std::endl;
        sendEvent(DeviceEvent(deviceId, "Temperature: " + std::to_string(temperature)));
    }
};

class SmartCamera : public SmartDevice {
    bool isRecording;

public:
    SmartCamera(int id, std::string name)
        : SmartDevice(id, name), isRecording(false) {
        connectionTypes = { ConnectionType::WiFi };
        powerSources = { PowerSource::Mains };
    }

    void handleEvent(const Event& event) override {
        if (event.getType() == EventType::APP_EVENT) {
            const AppEvent& appEvent = static_cast<const AppEvent&>(event);
            if (appEvent.getData() == "TOGGLE_RECORDING") {
                isRecording = !isRecording;
                std::cout << "Camera " << name << " recording "
                    << (isRecording ? "started" : "stopped") << std::endl;

                sendEvent(AppEvent(deviceId,
                    "Recording " + std::string(isRecording ? "started" : "stopped"),
                    Direction::TO_APP));
            }
        }
    }

    void detectMotion() {
        std::cout << "Motion detected by camera " << name << std::endl;
        sendEvent(DeviceEvent(deviceId, "Motion detected"));
    }
};
class Room {
    std::string nameLabel;
    std::string miscInfo;
    std::vector<SmartDevice*> deviceList;

public:
    Room(const std::string& name, const std::string& info = "")
        : nameLabel(name), miscInfo(info) {
        std::cout << "Created Room: " << nameLabel << std::endl;
    }

    void addDevice(SmartDevice* device) {
        deviceList.push_back(device);
        std::cout << "Added device " << device->getName() << " to room " << nameLabel << std::endl;
    }

    void removeDevice(SmartDevice* device) {
        auto it = std::find(deviceList.begin(), deviceList.end(), device);
        if (it != deviceList.end()) {
            deviceList.erase(it);
            std::cout << "Removed device " << device->getName() << " from room " << nameLabel << std::endl;
        }
    }
};

class PhoneApp {
    int appId;
    MainNode* mainNode;

public:
    PhoneApp(int id) : appId(id), mainNode(nullptr) {
        std::cout << "Created PhoneApp with ID: " << appId << std::endl;
    }

    void setMainNode(MainNode* node) { mainNode = node; }

    void sendEvent(const AppEvent& event);
    

    const void receiveEvent(const AppEvent& event) {
        std::cout << "PhoneApp " << appId << " received event: " << event.toJson() << std::endl;
    }
};

class Human {
    std::string name;
    PhoneApp* phone;

public:
    Human(const std::string& n, PhoneApp* p) : name(n), phone(p) {
        std::cout << "Created Human: " << name << std::endl;
    }

    void makeAction(int deviceId, const std::string& action) {
        std::cout << name << " initiating action: " << action << " for device " << deviceId << std::endl;
        phone->sendEvent(AppEvent(deviceId, action, Direction::FROM_APP));
    }

    const void acceptResponse(const AppEvent& event) {
        std::cout << name << " received response: " << event.toJson() << std::endl;
    }
};

class MainNode {
    std::vector<std::unique_ptr<SmartDevice>> pairedDevices;
    std::vector<PhoneApp*> linkedApps;
    std::vector<std::unique_ptr<Room>> roomList;

public:
    MainNode() {
        std::cout << "MainNode initialized" << std::endl;
    }

    void addDevice(std::unique_ptr<SmartDevice> device) {
        std::cout << "MainNode: Adding device " << device->getName() << std::endl;
        device->setMainNode(this);
        pairedDevices.push_back(std::move(device));
    }

    void linkApp(PhoneApp* app) {
        std::cout << "MainNode: Linking PhoneApp" << std::endl;
        app->setMainNode(this);
        linkedApps.push_back(app);
    }

    void addRoom(std::unique_ptr<Room> room) {
        std::cout << "MainNode: Adding room" << std::endl;
        roomList.push_back(std::move(room));
    }

    void handleEvent(const Event& event) {
        std::cout << "MainNode: Processing event: " << event.toJson() << std::endl;

        if (event.getType() == EventType::APP_EVENT) {
            const AppEvent& appEvent = static_cast<const AppEvent&>(event);

            if (appEvent.getDirection() == Direction::FROM_APP) {
                // Forward to device
                for (const auto& device : pairedDevices) {
                    if (device->getId() == event.getDeviceId()) {
                        device->handleEvent(event);
                        break;
                    }
                }
            }
            else {
                // Forward to apps
                for (auto app : linkedApps) {
                    app->receiveEvent(appEvent);
                }
            }
        }
        else if (event.getType() == EventType::DEVICE_EVENT) {
            // Forward device events to apps
            AppEvent appEvent(event.getDeviceId(),
                static_cast<const DeviceEvent&>(event).toJson(),
                Direction::TO_APP);
            for (auto app : linkedApps) {
                app->receiveEvent(appEvent);
            }
        }
    }
};

void PhoneApp::sendEvent(const AppEvent& event) {
    std::cout << "PhoneApp " << appId << " sending event: " << event.toJson() << std::endl;
    if (mainNode) mainNode->handleEvent(event);
}


// Now we can implement SmartDevice::sendEvent
void SmartDevice::sendEvent(const Event& event) {
    std::cout << "Device " << name << " sending event: " << event.toJson() << std::endl;
    if (mainNode) mainNode->handleEvent(event);
}
int main() {
    std::cout << "\n=== Initializing Smart Home System ===\n" << std::endl;

    // Create main node
    MainNode mainNode;

    // Create rooms
    auto livingRoom = std::make_unique<Room>("Living Room");
    auto bedroom = std::make_unique<Room>("Bedroom");

    // Keep raw pointers for interaction before moving devices to MainNode
    auto* lightPtr = new SmartLight(1, "Living Room Light");
    auto* thermostatPtr = new SmartThermostat(2, "Home Thermostat");
    auto* cameraPtr = new SmartCamera(3, "Security Camera");

    // Create phone app and human
    PhoneApp app(1);
    Human user("John", &app);

    // Setup system
    mainNode.linkApp(&app);

    // Add devices to rooms (using raw pointers)
    livingRoom->addDevice(lightPtr);
    livingRoom->addDevice(cameraPtr);
    bedroom->addDevice(thermostatPtr);

    // Add rooms to main node
    mainNode.addRoom(std::move(livingRoom));
    mainNode.addRoom(std::move(bedroom));

    // Transfer device ownership to MainNode
    mainNode.addDevice(std::unique_ptr<SmartDevice>(lightPtr));
    mainNode.addDevice(std::unique_ptr<SmartDevice>(thermostatPtr));
    mainNode.addDevice(std::unique_ptr<SmartDevice>(cameraPtr));

    std::cout << "\n=== Starting User Interactions ===\n" << std::endl;

    // Example interactions
    std::cout << "\n--- Light Control Test ---\n";
    user.makeAction(1, "TOGGLE"); // Turn on the light
    std::this_thread::sleep_for(std::chrono::seconds(1));
    user.makeAction(1, "TOGGLE"); // Turn off the light
    std::this_thread::sleep_for(std::chrono::seconds(1));

    std::cout << "\n--- Thermostat Test ---\n";
    user.makeAction(2, "SET_TEMP:24.5"); // Set thermostat
    std::this_thread::sleep_for(std::chrono::seconds(1));

    std::cout << "\n--- Camera Test ---\n";
    user.makeAction(3, "TOGGLE_RECORDING"); // Start camera recording
    std::this_thread::sleep_for(std::chrono::seconds(1));

    std::cout << "\n=== System Running ===\n" << std::endl;

    // Simulate some automated events
    for (int i = 0; i < 3; i++) {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        // Simulate temperature fluctuation
        thermostatPtr->simulateTemperatureChange(22.0 + (float)(rand() % 4));

        if (i % 2 == 0) {
            // Simulate motion detection
            cameraPtr->detectMotion();
        }
    }

    std::cout << "\n=== End of Simulation ===\n" << std::endl;
    return 0;
}
