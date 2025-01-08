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
{ // event is passed back and forth between main node and devices, every node should have 
    long unsigned int timestamp;// time when event was sent in ms from epoch begining
    long unsigned int deviceId; // this one is for separating eventTypeId for different devicec
    long int eventTypeId;//event type is abstract number that specifies what does event do, every device has reserved
    virtual int getIntParam(short unsigned int paramId){
        //virtual func for getting parameter by specied id
    }
    virtual float getFloatParam(short unsigned int paramId){
        //virtual func for getting parameter by specied id
    }
    virtual std::string getStringParam(short unsigned int paramId){
        //virtual func for getting parameter by specied id
    }
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
        for(int i=0;i< this->pairedDevices.size();i++){
            if(device.UUID==pairedDevices[i].UUID){
                //found device to remove
                this->pairedDevices.erase (this->pairedDevices.begin()+i);
                break;
            }
        }
    }
    void addRoom(Room room)
    {
        this->roomList.push_back(room);
    }
    void removeRoom(Room room)
    {
        for(int i=0;i< this->roomList.size();i++){
            if(room.UUID==roomList[i].UUID){
                //found room to remove
                this->roomList.erase (this->roomList.begin()+i);
                break;
            }
        }
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
    unsigned int deviceId;
    unsigned long  int UUID;
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
    unsigned long  int UUID;
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
