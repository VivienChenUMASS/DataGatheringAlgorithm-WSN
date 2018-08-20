//
//  simulation.hpp
//
//  Created by Vivien Chen.
//  Copyright (c) Vivien Chen. All rights reserved.
//
//

#include <iostream>
#include <algorithm>
#include <assert.h>
#include <math.h>
#include <map>
#include <list>
#include <vector>
#include <fstream>
#include <iterator>
#include "MersenneTwister.hpp"

using namespace std;

typedef struct time{
    
    int day;
    int hour;
    int minute;
    
    bool operator != (const time& rhs) const
    {
        return this->day != rhs.day || this->hour != rhs.hour || this->minute != rhs.minute;
    }
    
    bool operator < (const time& rt) const{
        
        int lminute = this->day * 24 * 60 + this->hour * 60 + this->minute;
        int rminute = rt.day * 24 * 60 + rt.hour * 60 + rt.minute;
        if(lminute < rminute) return true;
        else return false;
    }
    
}TIME;

typedef struct position{
    
    float x;
    float y;
}POSITION;

typedef struct sub_Area{
    
    float width;
    float length;
}Sub_Area;

typedef struct subArea_NO{
    
    int rowNO;
    int columnNO;
    
}SubArea_NO;

typedef struct request_type{
    
    TIME time;    //request happen time
    POSITION request_position;
}REQUEST;

typedef struct request_messenge{
    
    int UserID;
    REQUEST req;
    float Last_Rep_Data;
    
}R_Messenge;

extern TIME Simulation_Start_Time;
extern TIME Simulation_Current_Time;

extern int NO_TimeLevels, NO_DataLevels, NO_EnergyLevels;
extern float Temperature_Max, Temperature_Min, Temperature_Difference_Min;
extern float Energy_Level_Interval;
extern float Delta_T;
extern float Operation_Period;

class SensorNet_System;
class User;
class Sensor;

class Request_Model{
    
    friend class SensorNet_System;
    friend class User;
    
private:
    float lambda;
    
public:
    void set_lambda(float lambda){ this->lambda = lambda;}
    
    float get_lambda(){return lambda;}
    float generate_PossionInterval();
    
};

class Energy_Harvest_Model{
    /* this version assumes the energy_harvest rate is always the same, just related to the time interval*/
private:
    //float energy_harvest_rate;
    
public:
    //void set_EnergyHarvest_Rate(float rate){ this->energy_harvest_rate = rate;}
    float get_EnergyHarvestRate(TIME);
    float get_Harvest_Energy(TIME, TIME);
    
    void Table_Caculation(User*, float, SensorNet_System*);
};

class Temperature_Model{
    
    friend class SensorNet_System;
    
private:
    float Current_Cloudiness; //range from -0.5 to 0.5; -0.5~0 means sunshine; 0~0.5 means cloudy/rainy
    
    float Current_Temperature; //For one request round, the temperature of the requested location, assume in the process of serving the request by the base station and the sensor nodes, the time is very very short that the temperature of requested location sensed is unchanged!!!
    int Current_Temperature_Flag = 0; //Used for marking this round's current temperature has been generated!!!0: has not been generated 1: has been generated!!!
    
    float time_component(float);
    float day_component(float);
    float cloudiness_component(float);
    
public:
    void set_Current_Cloudiness(){
        Current_Cloudiness = genrand_real2() - 0.5;
        assert(Current_Cloudiness <= 0.5 && Current_Cloudiness >= -0.5);
    }
    float get_Current_Cloudiness(){ return Current_Cloudiness;}
    void set_Current_Temperature(float temperature){ Current_Temperature = temperature;}
    float get_Current_Temperature(){ return Current_Temperature;}
    void set_Current_Temperature_Flag(int flag){ Current_Temperature_Flag = flag;}
    int get_Current_Temperature_Flag(){ return Current_Temperature_Flag;}
    //float generate_temperature(float,float,float,float);
    float generate_temperature();
    
    friend void Simulation(SensorNet_System*);
};

class User{
    
    friend class SensorNet_System;
    
private:
    int ID;
    float Period;
    POSITION Area_Start;
    POSITION Area_End;
    Request_Model User_Request_Model;
    
public:
    int get_ID(){ return ID; }
    void set_ID(int id){ ID = id; }
    void set_Period(float period){ Period = period;}
    void set_User_Request_Model(float lambda){ User_Request_Model.set_lambda(lambda);}
    void set_User_Area(POSITION Area_Start, POSITION Area_End){ this->Area_Start = Area_Start; this->Area_End = Area_End;}
    
    Request_Model get_RequestModelP(){return User_Request_Model;}
    REQUEST* generate_nextRequest();
    float Value_of_Data(float, float); //this value of accuracy is defined by different users
    
    friend void Table_Caculation(User*, float, SensorNet_System*);
};

class BaseStation{
    
    friend class SensorNet_System;
    
private:
    
    POSITION Area_Start;
    POSITION Area_End;
    int UserNumber;
    int SensorNumber;
    map<int, Sensor*> SensorID_SensorP; //a map stores all the sensors' pointers
    map<int, REQUEST*> UserID_RequestHead;//a map stores all the users' request queue head pointer
    vector<int> SensorID_Available_Algorithm1; //a vector that stores IDs of the sensors that haven't been requested yet for one request
    vector<int> SensorID_Available_Algorithm2;
    vector<int> SensorID_Available_Algorithm3;
    int FirstReq_UserID; //the user id of the first request that is being handling by the base station currently
    REQUEST* FirstReq = NULL;
    int SensorID_Requested;
    Sub_Area subArea;
    SubArea_NO subAreaNO;
    int LastRepDataSensorNO_Flag; //initially = 0; 0: Neither of the sensors transmit back a new data; 1: sensor id 1 transmits back a new data; 2: sensor id 2 transmits back a new data; different algorithms will share the same flag, so be careful!!
    vector<float> LastRepData_Algorithm1;
    vector<TIME> LastRepDataTime_Algorithm1;
    vector<float> LastRepData_Algorithm2;
    vector<TIME> LastRepDataTime_Algorithm2;
    vector<float> LastRepData_Algorithm3;
    vector<TIME> LastRepDataTime_Algorithm3;
    
    vector<float> Reward_Gained_Algorithm;
    int Total_Request_NO;
    int No_Data_TransBack_NO_Algorithm[3];
    vector<int> Actual_Action_Algorithm1;
    vector<int> Actual_Action_Algorithm2;
    vector<int> Actual_Action_Algorithm3;
    vector<float> Every_round_added_reward_Algorithm1;
    vector<float> Every_round_added_reward_Algorithm2;
    vector<float> Every_round_added_reward_Algorithm3;
    
public:
    BaseStation(){Total_Request_NO = 0; No_Data_TransBack_NO_Algorithm[0] = 0; No_Data_TransBack_NO_Algorithm[1] = 0; No_Data_TransBack_NO_Algorithm[2] = 0;}
    void set_AreaStart(float x, float y){ Area_Start.x = x; Area_Start.y = y; }
    void set_AreaEnd(float x, float y){ Area_End.x = x; Area_End.y = y; }
    void set_subArea(float width, float length){ subArea.width = width; subArea.length = length;}
    void set_subAreaNo(int rowN, int columnN){ subAreaNO.rowNO = rowN; subAreaNO.columnNO = columnN;}
    void set_Reward_Gained_Algorithm(int Algorithm_NO, float reward){this->Reward_Gained_Algorithm[Algorithm_NO] = reward;}
    
    void add_Reward_Algorithm(int Algorithm_NO, float added_reward){this->Reward_Gained_Algorithm[Algorithm_NO] = this->Reward_Gained_Algorithm[Algorithm_NO] + added_reward;}
    void add_Actual_Action_Algorithm(int Algorithm_NO, int Actual_Action_NO){
        switch (Algorithm_NO) {
            case 1:
                Actual_Action_Algorithm1.push_back(Actual_Action_NO);
                break;
            case 2:
                Actual_Action_Algorithm2.push_back(Actual_Action_NO);
                break;
            case 3:
                Actual_Action_Algorithm3.push_back(Actual_Action_NO);
                break;
            default:
                cout << "No Algorithm Case Match!" << endl;
                exit(1);
        }
    }
    void record_added_reward_everyRound_Algorithm(int Algorithm_NO, float added_reward_this_round){
        switch (Algorithm_NO) {
            case 1:
                Every_round_added_reward_Algorithm1.push_back(added_reward_this_round);
                break;
            case 2:
                Every_round_added_reward_Algorithm2.push_back(added_reward_this_round);
                break;
            case 3:
                Every_round_added_reward_Algorithm3.push_back(added_reward_this_round);
                break;
            default:
                cout << "No Algorithm Case Match!" << endl;
                exit(1);
        }
    }
    
    int get_UserNumber(){ return UserNumber; }
    int get_SensorNumber(){ return SensorNumber; }
    void set_UserNumber(int number){ UserNumber = number; }
    void set_SensorNumber(int number){ SensorNumber = number; }
    int get_SensorID_Requested(){ return SensorID_Requested;}
    REQUEST* get_FirstReq(){ return FirstReq;}
    int get_FirstReq_UserID(){ return FirstReq_UserID;}
    vector<int>* get_SensorAvailable_Algorithm(int Algorithm_NO){
        switch (Algorithm_NO) {
            case 1:
                return &SensorID_Available_Algorithm1;
            case 2:
                return &SensorID_Available_Algorithm2;
            case 3:
                return &SensorID_Available_Algorithm3;
            default:
                cout << "No Algorithm Case Match!" << endl;
                exit(1);
        }
    }
    
    vector<float>* access_LastRepData_Algorithm(int Algorithm_NO){
        switch (Algorithm_NO) {
            case 1:
                return &LastRepData_Algorithm1;
            case 2:
                return &LastRepData_Algorithm2;
            case 3:
                return &LastRepData_Algorithm3;
            default:
                cout << "No Algorithm Case Match!" << endl;
                exit(1);
        }
    }
    
    vector<TIME>* access_LastRepDataTime_Algorithm(int Algorithm_NO){
        switch (Algorithm_NO) {
            case 1:
                return &LastRepDataTime_Algorithm1;
            case 2:
                return &LastRepDataTime_Algorithm2;
            case 3:
                return &LastRepDataTime_Algorithm3;
            default:
                cout << "No Algorithm Case Match!" << endl;
                exit(1);
        }
    }
    
    map<int, Sensor*>* access_SensorID_SensorP(){ return &SensorID_SensorP;}
    map<int, REQUEST*>* access_UserID_RequestHead(){ return &UserID_RequestHead;}
    
    void update_LastRepData_Algorithm(int, float, int);
    void update_LastRepDataTime_Algorithm(int, TIME, int);
    
    void find_firstRequest();
    void send_Request_Algorithm(int); //send the request to the right sensor
    
    void delete_Req();
    
    void Table_Caculation(User*, float, SensorNet_System*);
    
    friend void Simulation(SensorNet_System*);
    
};

class Sensor{
    
    friend class SensorNet_System;
    friend class BaseStation;
    
private:
    int ID;
    POSITION Sensor_Position;
    float Sensing_Range;
    float Capacity;
    float Transmit_Action_Energy;
    float Energy_Remain_Algorithm1;
    float Energy_Remain_Algorithm2;
    float Energy_Remain_Algorithm3;
    Energy_Harvest_Model* EHarvestM_p;
    Temperature_Model* TM_p;
    TIME Last_EHarvest_UpdateT_Algorithm1;
    TIME Last_EHarvest_UpdateT_Algorithm2;
    TIME Last_EHarvest_UpdateT_Algorithm3;
    //int Round_Sensed_Flag; //used to indicate whether the sensor has sensed the current data in this request round!
    float Current_Sensed_Data;
    int Trans_Decision; //1:transmit 0:not transmit
    vector<vector<vector<vector<int>>>> decision_Table_Algorithm1;
    float Total_Request_NO_Algorithm1;
    float Total_Request_NO_Algorithm2;
    float Total_Request_NO_Algorithm3;
    float Total_NotTrans_NO_Algorithm1;
    float Total_NotTrans_NO_Algorithm2;
    float Total_NotTrans_NO_Algorithm3;
    float Last_Report_Data_Algorithm1;
    float Last_Report_Data_Algorithm2;
    float Last_Report_Data_Algorithm3;
public:
    //Sensor():Round_Sensed_Flag(0){} //0:means the sensor has not sensed yet 1:means the sensor has sensed this round!
    void set_ID(int id){ ID = id; }
    void set_Sensor_Position(float x, float y){ this->Sensor_Position.x = x; this->Sensor_Position.y = y;}
    void set_Sensing_Range(float r){this->Sensing_Range = r;}
    POSITION get_Sensor_Position(){ return Sensor_Position;}
    void set_Capacity(float capacity){ this->Capacity = capacity;}
    void set_Transmit_Action_Energy(float TransActEnergy){ this->Transmit_Action_Energy = TransActEnergy;}
    void set_Energy_Remain_Algorithm1(float Energy_Remain){ this->Energy_Remain_Algorithm1 = Energy_Remain;}
    void set_Energy_Remain_Algorithm2(float Energy_Remain){ this->Energy_Remain_Algorithm2 = Energy_Remain;}
    void set_Energy_Remain_Algorithm3(float Energy_Remain){ this->Energy_Remain_Algorithm3 = Energy_Remain;}
    void set_EHarvestM_p(Energy_Harvest_Model* p){ this->EHarvestM_p = p;}
    void set_TM_p(Temperature_Model* p){ this->TM_p = p;}
    void set_Last_EHarvest_UpdateT_Algorithm1(TIME time){ this->Last_EHarvest_UpdateT_Algorithm1 = time;}
    void set_Last_EHarvest_UpdateT_Algorithm2(TIME time){ this->Last_EHarvest_UpdateT_Algorithm2 = time;}
    void set_Last_EHarvest_UpdateT_Algorithm3(TIME time){ this->Last_EHarvest_UpdateT_Algorithm3 = time;}
    void set_Current_Sensed_Data(float CurrentSensedData){ this->Current_Sensed_Data = CurrentSensedData;}
    void set_Total_Req_NO_Algorithm(int Algorithm_NO, int n){
        switch (Algorithm_NO) {
            case 1:
                Total_Request_NO_Algorithm1 = n;
                break;
            case 2:
                Total_Request_NO_Algorithm2 = n;
                break;
            case 3:
                Total_Request_NO_Algorithm3 = n;
                break;
            default:
                cout << "NO ALGORITHM CASE MATCH!" << endl;
                exit(1);
        }
    }
    void set_Total_NotTrans_NO_Algorithm(int Algorithm_NO, int n){
        switch (Algorithm_NO) {
            case 1:
                Total_NotTrans_NO_Algorithm1 = n;
                break;
            case 2:
                Total_NotTrans_NO_Algorithm2 = n;
                break;
            case 3:
                Total_NotTrans_NO_Algorithm3 = n;
                break;
            default:
                cout << "NO ALGORITHM CASE MATCH!" << endl;
                exit(1);
        }
        
    }
    
    float get_capacity(){return Capacity;}
    float get_Current_Sensed_Data(){ return Current_Sensed_Data;}
    vector<vector<vector<vector<int>>>>* access_Decision_Table(){ return &decision_Table_Algorithm1;}
    float get_TransmitActEnergy(){return Transmit_Action_Energy;}
    int get_TransDecision(){ return Trans_Decision;}
    float get_Total_Req_NO_Algorithm(int Algorithm_NO){
        switch (Algorithm_NO) {
            case 1:
                return Total_Request_NO_Algorithm1;
            case 2:
                return Total_Request_NO_Algorithm2;
            case 3:
                return Total_Request_NO_Algorithm3;
            default:
                cout << "NO ALGORITHM CASE MATCH!" << endl;
                exit(1);
        }
    }
    float get_Total_NotTrans_NO_Algorithm(int Algorithm_NO){
        switch (Algorithm_NO) {
            case 1:
                return Total_NotTrans_NO_Algorithm1;
            case 2:
                return Total_NotTrans_NO_Algorithm2;
            case 3:
                return Total_NotTrans_NO_Algorithm3;
            default:
                cout << "NO ALGORITHM CASE MATCH!" << endl;
                exit(1);
        }
    }
    
    void update_Last_Report_Data_Algorithm(int, float);
    
    void process_request_Algorithm(int, REQUEST);
    
    float Sense_Current_Data(POSITION);
    
    void Table_Caculation(User*, float, SensorNet_System*);
    friend void Simulation(SensorNet_System*);
};

class SensorNet_System{
    //in this system, there will have 2 sensors and 1 user and 1 base station
private:
    int UserNumber;
    int SensorNumber;
    float system_operationTime;
    map<int, Sensor*> SensorMap;
    map<int, User*> UserMap;
    BaseStation myBaseStation; //one system will only have one base station
    Energy_Harvest_Model myEHarvestModel;
    Temperature_Model myTemperatureModel;
    
    POSITION Area_Start;
    POSITION Area_End;
    Sub_Area Subarea;
    SubArea_NO Subarea_NO;
    float Area_Overlap;
    float Area_WithinSenseRange;
    
public:
    ~SensorNet_System(){
        for (std::map<int, Sensor*>::iterator it = SensorMap.begin(); it != SensorMap.end(); ++it) {
            delete it->second;
        }
        for (std::map<int, User*>::iterator it = UserMap.begin(); it != UserMap.end(); ++it) {
            delete it->second;
        }
    }
    map<int, User*>* get_UserMap(){ return &UserMap;}
    map<int, Sensor*>* get_SensorMap(){ return &SensorMap;}
    int get_UserNumber(){ return UserNumber;}
    float get_operationTime(){ return system_operationTime;}
    BaseStation* access_BaseStation(){ return &myBaseStation;}
    Temperature_Model* access_TemperatureModel(){
        //cout << "the address of myTemperatureModel now is : " << &myTemperatureModel << endl;
        return &myTemperatureModel;}
    
    void Initialize_Sensor(Sensor*, int, int, float, float, float, Energy_Harvest_Model*, Temperature_Model*);
    void Initialize_User(User*, float, float);
    void Initialize_BaseStation();
    void Initialize_System(int, float, float, int, float, float, POSITION, POSITION, Sub_Area);
    
    void System_Recycle();
    
    friend void Table_Caculation(User*, float, SensorNet_System*);
    friend void Simulation(SensorNet_System*);
    friend int main();
};

void Table_Caculation(User*, float, SensorNet_System*);

void Simulation(SensorNet_System*);

