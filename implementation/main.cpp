#include <string>
#include <vector>

enum ConnectionType
{
    WiFi,
    ZigBee,
    Z_Wave,
    Bluetooth,
    Ethernet
};

enum PowerSource
{
    Battery,
    Mains,
    Solar,
    Other
};

struct Event
{ // event is passed back and forth between main node and devices
    std::string name;
    std::string description;
    std::string data;
};

class Human;

class mainNode
{
public:
    std::vector<SmartDevice> pairedDevices;
    std::vector<Room> roomList;
    void addDevice(SmartDevice device)
    {
        this->pairedDevices.push_back(device);
    }
    void removeDevice(SmartDevice device)
    {
        this->pairedDevices.erase(device);
    }
    void addRoom(Room room)
    {
        this->roomList.push_back(room);
    }
    void removeRoom(Room room)
    {
        this->roomList.erase(room);
    }
    void pairDevice(SmartDevice device)
    {
        device.isPairedTo = this;
    }
    void unpairDevice(SmartDevice device)
    {
        device.isPairedTo = nullptr;
    }
    void handleEvent(Event event)
    {
        // this is a virtual function that will be overriden by the derived classes
        // is responsible for handling ogf any events
    }
    void sendEvent(Event event, SmartDevice device)
    {
        device.handleEvent(event);
    }
};

class SmartDevice
{
    // base class for all smart devices
public:
    std::string name;
    ConnectionType connectionType;
    PowerSource poweredBy;
    mainNode *isPairedTo;

    virtual void handleEvent(Event event) {
        // this is a virtual function that will be overriden by the derived classes
        // is responsible for handling ogf any events
    };
    bool operator==(SmartDevice device)
    {
        return this->name == device.name;
    }

private:
    void sendEvent(Event event) {
        // this->isPairedTo->handleEvent(event);
    };
};

class Room
{
public:
    std::string nameLabel;
    std::string miscInfo;
    std::vector<SmartDevice> smartDevices;
    void addDevice(SmartDevice device)
    {
        this->smartDevices.push_back(device);
    }
    void removeDevice(SmartDevice device)
    {
        this->smartDevices.erase(device);
    }
};

class PhoneApp
{
    // should have user property of human type
    Human user;
    void transmitCommand()
    {
        // this is a virtual function that will be overriden by the derived classes
        // is responsible for sending commands to the main node
    }
    void recieveUpdates()
    {
        // this is a virtual function that will be overriden by the derived classes
        // is responsible for recieving updates from the main node
    }
    void transmitCommand()
    {
    }
    void notifyUser()
    {
        // this is a virtual function that will be overriden by the derived classes
        // is responsible for notifying the user of any updates
    }
};

class Human
{
public:
    std::string name;
    PhoneApp phone;
    void recieveFeedback() {}
    void makeAction()
    {
        this->phone;
    }
};
