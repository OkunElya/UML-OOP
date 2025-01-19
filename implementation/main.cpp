#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <chrono>
#include <thread>
#include <algorithm>

// Enums
enum class ConnectionType
{
    WiFi,
    Zigbee,
    ZWave,
    Bluetooth,
    Ethernet
};
enum class PowerSource
{
    Battery,
    Mains,
    Solar,
    Other
};
enum class EventType
{
    DEVICE_EVENT,
    APP_EVENT
};
enum class Direction
{
    TO_APP,
    TO_DEVICE,
    TO_NODE
};

// Forward declarations
class MainNode;
class Event;
class AppEvent;
class DeviceEvent;

// Base Event class
class Event
{
protected:
    EventType type;
    int deviceId;
    long long timestamp;
    std::string data;
    Direction direction;

public:
    Event(EventType t, int id, std::string data) : type(t), deviceId(id)
    {
        timestamp = std::chrono::system_clock::now().time_since_epoch().count();
    }

    Event() {}
    Event(DeviceEvent& devE);
    Event(AppEvent& devE);

    EventType getType() const { return type; }
    int getDeviceId() const { return deviceId; }
    Direction getDirection() const { return direction; }
    std::string getData() const { return data; }

    std::string toJson() const
    {
        return "{ type: APP_EVENT, deviceId: " + std::to_string(deviceId) +
            ", direction: " + (direction == Direction::TO_APP ? "TO_APP" : "TO_DEVICE") +
            ", data: " + data + " }";
    }
};

class DeviceEvent : public Event
{
public:
    DeviceEvent(Event& event)
    {
        DeviceEvent(event.getDeviceId(), event.getData(), event.getDirection());
    }
    DeviceEvent(int deviceId, std::string data, Direction dir)
        : Event(EventType::DEVICE_EVENT, deviceId, data)
    {
        type = EventType::DEVICE_EVENT;
        std::cout << "DeviceEvent created: " << toJson() << std::endl;
    }
};

class AppEvent : public Event
{
public:
    AppEvent(Event& event)
    {
        AppEvent(event.getDeviceId(), event.getData(), event.getDirection());
    }
    AppEvent(int deviceId, std::string data, Direction dir)
        : Event(EventType::DEVICE_EVENT, deviceId, data)
    {
        type = EventType::APP_EVENT;
        std::cout << "AppEvent created: " << toJson() << std::endl;
    }
};

Event::Event(DeviceEvent& devE)
{
    timestamp = std::chrono::system_clock::now().time_since_epoch().count();
    type = EventType::DEVICE_EVENT;
    data = devE.getData();
    deviceId = devE.getDeviceId();
    direction = devE.getDirection();
}
Event::Event(AppEvent& devE)
{
    timestamp = std::chrono::system_clock::now().time_since_epoch().count();
    type = EventType::DEVICE_EVENT;
    data = devE.getData();
    deviceId = devE.getDeviceId();
    direction = devE.getDirection();
}

// Base calss for smart devices
class SmartDevice
{
protected:
    int deviceId;
    std::string name;
    std::vector<ConnectionType> connectionTypes;
    std::vector<PowerSource> powerSources;
    MainNode* mainNode;

public:
    SmartDevice(int id, std::string n) : deviceId(id), name(n), mainNode(nullptr)
    {
        std::cout << "Created SmartDevice: " << name << " (ID: " << deviceId << ")" << std::endl;
    }

    virtual ~SmartDevice() = default;

    void setMainNode(MainNode* node) { mainNode = node; }
    int getId() const { return deviceId; }
    std::string getName() const { return name; }

    virtual void handleEvent(const DeviceEvent* event) = 0;
    void sendEvent(const DeviceEvent* event); // Implementation after MainNode definition
};

class SmartLight : public SmartDevice
{
    bool isOn;
public:
    SmartLight(int id, std::string name)
        : SmartDevice(id, name), isOn(false)
    {
        connectionTypes = { ConnectionType::WiFi };
        powerSources = { PowerSource::Mains };
    }

    void handleEvent(const DeviceEvent* event) override
    {

        if (event->getData() == "TOGGLE")
        {
            isOn = !isOn;
            std::cout << "Light " << name << " turned " << (isOn ? "ON" : "OFF") << std::endl;

            // Send feedback
            sendEvent(new DeviceEvent(deviceId,
                "Light state changed to " + std::string(isOn ? "ON" : "OFF"),
                Direction::TO_APP));
        }
    }
};

class SmartThermostat : public SmartDevice
{
    float temperature;
    float targetTemp;

public:
    SmartThermostat(int id, std::string name)
        : SmartDevice(id, name), temperature(20.0f), targetTemp(22.0f)
    {
        connectionTypes = { ConnectionType::WiFi, ConnectionType::ZWave };
        powerSources = { PowerSource::Mains };
    }

    void handleEvent(const DeviceEvent* event) override
    {

        if (event->getData().find("SET_TEMP:") == 0)
        {
            targetTemp = std::stof(event->getData().substr(9));
            std::cout << "Thermostat " << name << " target temperature set to "
                << targetTemp << "C" << std::endl;

            sendEvent(new DeviceEvent(deviceId,
                "Target temperature set to " + std::to_string(targetTemp),
                Direction::TO_APP));
        }
    }

    void simulateTemperatureChange(float newTemp)
    {
        temperature = newTemp;
        std::cout << "Temperature changed to " << temperature << "°C" << std::endl;
        sendEvent(new DeviceEvent(deviceId, "Temperature: " + std::to_string(temperature), Direction::TO_NODE));
    }
};

class SmartCamera : public SmartDevice
{
    bool isRecording;

public:
    SmartCamera(int id, std::string name)
        : SmartDevice(id, name), isRecording(false)
    {
        connectionTypes = { ConnectionType::WiFi };
        powerSources = { PowerSource::Mains };
    }

    void handleEvent(const DeviceEvent* event) override
    {
        if (event->getData() == "TOGGLE_RECORDING")
        {
            isRecording = !isRecording;
            std::cout << "Camera " << name << " recording "
                << (isRecording ? "started" : "stopped") << std::endl;

            sendEvent(new DeviceEvent(deviceId,
                "Recording " + std::string(isRecording ? "started" : "stopped"),
                Direction::TO_APP));
        }
    }

    void detectMotion()
    {
        std::cout << "Motion detected by camera " << name << std::endl;
        sendEvent(new DeviceEvent(deviceId, "Motion detected", Direction::TO_NODE));
    }
};

// room is a storage for smartDevices
class Room
{
    std::string nameLabel;
    std::string miscInfo;
    std::vector<SmartDevice*> deviceList;

public:
    Room(const std::string& name, const std::string& info = "")
        : nameLabel(name), miscInfo(info)
    {
        std::cout << "Created Room: " << nameLabel << std::endl;
    }

    void addDevice(SmartDevice* device)
    {
        deviceList.push_back(device);
        std::cout << "Added device " << device->getName() << " to room " << nameLabel << std::endl;
    }

    void removeDevice(SmartDevice* device)
    {
        auto it = std::find(deviceList.begin(), deviceList.end(), device);
        if (it != deviceList.end())
        {
            deviceList.erase(it);
            std::cout << "Removed device " << device->getName() << " from room " << nameLabel << std::endl;
        }
    }
};

// an interface via which human interracts with the system
class PhoneApp
{
    int appId;
    MainNode* mainNode;

public:
    PhoneApp(int id) : appId(id), mainNode(nullptr)
    {
        std::cout << "Created PhoneApp with ID: " << appId << std::endl;
    }

    void setMainNode(MainNode* node) { mainNode = node; }

    void sendEvent(AppEvent* event);

    const void receiveEvent(AppEvent* event)
    {
        std::cout << "PhoneApp " << appId << " received event: " << event->toJson() << std::endl;
    }
};

class Human
{
    std::string name;
    PhoneApp* phone;

public:
    Human(const std::string& n, PhoneApp* p) : name(n), phone(p)
    {
        std::cout << "Created Human: " << name << std::endl;
    }

    void makeAction(int deviceId, const std::string& action)
    {
        std::cout << name << " initiating action: " << action << " for device " << deviceId << std::endl;
        phone->sendEvent(new AppEvent(deviceId, action, Direction::TO_DEVICE));
    }

    const void acceptResponse(const AppEvent* event)
    {
        std::cout << name << " received response: " << event->toJson() << std::endl;
    }
};

// something like a server
class MainNode
{
    std::vector<SmartDevice*> pairedDevices;
    std::vector<PhoneApp*> linkedApps;
    std::vector<Room*> roomList;

public:
    MainNode()
    {
        std::cout << "MainNode initialized" << std::endl;
    }

    void addDevice(SmartDevice* device)
    {
        std::cout << "MainNode: Adding device " << device->getName() << std::endl;
        device->setMainNode(this);
        pairedDevices.emplace_back(device);
    }

    void linkApp(PhoneApp* app)
    {
        std::cout << "MainNode: Linking PhoneApp" << std::endl;
        app->setMainNode(this);
        linkedApps.emplace_back(app);
    }

    void addRoom(Room* room)
    {
        std::cout << "MainNode: Adding room" << std::endl;
        roomList.emplace_back(std::move(room));
    }

    void handleEvent(Event* event)
    {
        if (event->getType() == EventType::APP_EVENT)
        {
            std::cout << "MainNode: Processing event: " << event->toJson() << std::endl;
            if (event->getDirection() == Direction::TO_DEVICE)
            {
                // Forward to device
                for (const auto& device : pairedDevices)
                {
                    if (device->getId() == event->getDeviceId())
                    { 
                        device->handleEvent((DeviceEvent*)event);
                        break;
                    }
                }
            }
            else
            {
                // Forward to apps
                std::cout << "Node recieves event " << event->toJson();
            }
        }
        else
        {
            for (auto app : linkedApps)
            {
                app->receiveEvent((AppEvent*)event);
            }
        }
    }
};

void PhoneApp::sendEvent(AppEvent* appEvent)
{
    std::cout << "PhoneApp " << appId << " sending event: " << appEvent->toJson() << std::endl;
    if (mainNode)
        mainNode->handleEvent((Event*)appEvent);
}

// Implemented later because mainNode wasn't defined yet
void SmartDevice::sendEvent(const DeviceEvent* event)
{
    std::cout << "Device " << name << " sending event: " << event->toJson() << std::endl;
    if (mainNode)
        mainNode->handleEvent((Event*)event);
}
int main()
{
    std::cout << "\n=== Initializing Smart Home System ===\n"
        << std::endl;

    // Create main node
    MainNode mainNode;

    // Create rooms
    auto livingRoom = new Room("Living Room");
    auto bedroom = new Room("Bedroom");

    auto* lightPtr = new SmartLight(1, "Living Room Light");
    auto* thermostatPtr = new SmartThermostat(2, "Home Thermostat");
    auto* cameraPtr = new SmartCamera(3, "Security Camera");
    // add devices to the main node
    mainNode.addDevice(lightPtr);
    mainNode.addDevice(thermostatPtr);
    mainNode.addDevice(cameraPtr);
    // Create phone app and human, link app to the node
    PhoneApp app(1);
    Human user("Aboba", &app);
    mainNode.linkApp(&app);

    // Add devices to rooms
    livingRoom->addDevice(lightPtr);
    livingRoom->addDevice(cameraPtr);
    bedroom->addDevice(thermostatPtr);

    // Add rooms to main node
    mainNode.addRoom(livingRoom);
    mainNode.addRoom(bedroom);

    std::cout << "\n=== Starting User Interactions ===\n"
        << std::endl;

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

    std::cout << "\n=== System Running ===\n"
        << std::endl;

    // Simulate some automated events
    for (int i = 0; i < 3; i++)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        // Simulate temperature fluctuation
        thermostatPtr->simulateTemperatureChange(22.0 + (float)(rand() % 4));

        if (i % 2 == 0)
        {
            // Simulate motion detection
            cameraPtr->detectMotion();
        }
    }

    std::cout << "\n=== End of Simulation ===\n"
        << std::endl;
    return 0;
}
