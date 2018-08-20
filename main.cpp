//
//  main.cpp
//
//  Created by Vivien Chen.
//  Copyright (c) Vivien Chen. All rights reserved.
//

#include <iostream>
#include <algorithm>
#include <fstream>
#include <assert.h>
#include <math.h>
#include <vector>
#include "simulation.hpp"

using namespace std;

int NO_TimeLevels, NO_DataLevels, NO_EnergyLevels;
float Temperature_Max, Temperature_Min, Temperature_Difference_Min;
float Energy_Level_Interval;
float Delta_T;
float Operation_Period;
TIME Simulation_Start_Time; //this varible will be used in both the calculation part and the simulation part!!!
TIME Simulation_Current_Time; //this variable will only be used in the simulation part

int main(){
    
    Simulation_Start_Time.day = 22;
    Simulation_Start_Time.hour = 0;
    Simulation_Start_Time.minute = 0;
    
    POSITION Area_Start, Area_End;
    Area_Start.x = 0;
    Area_Start.y = 0;
    Area_End.x = 12; // length unit
    Area_End.y = 8; // width unit
    
    Sub_Area sub_area;
    sub_area.width = 1;
    sub_area.length = 1;
    
    Operation_Period = 1000 * 60; //here, will be seconds
    Delta_T = 1 * 60; //minute * 60(s)
    
    float capacity = 10; //J
    float trans_energy = 1;
    Energy_Level_Interval = 0.02;
    
    Temperature_Max = 30; //c
    Temperature_Min = -30; //c
    Temperature_Difference_Min = 2; //c
    
    float Lambda = 200; 
    
    NO_TimeLevels = Operation_Period / Delta_T + 1;
    cout << "Number of Time Levels will be" << NO_TimeLevels << endl;
    NO_EnergyLevels = capacity / Energy_Level_Interval + 1;
    cout << "Number of Energy Levels will be" << NO_EnergyLevels << endl;
    NO_DataLevels = (Temperature_Max - Temperature_Min) / Temperature_Difference_Min + 1;
    cout << "Number of Data Levels will be" << NO_DataLevels << endl;
    
    for (int i = 0; i < 4; i++) {
        
        //initialize the system for the simulation part
        init_genrand(time(NULL));
        SensorNet_System* SensorNet_System00 = new SensorNet_System(); //this system will include 1 sensors and 1 user
        SensorNet_System00->Initialize_System(1, capacity, trans_energy, 1, Operation_Period, Lambda, Area_Start, Area_End,sub_area); //sensors NO = 1, user NO = 1
        
        Table_Caculation(SensorNet_System00->UserMap[1], 0, SensorNet_System00);
        
        for (map<int, Sensor*>::iterator it = SensorNet_System00->get_SensorMap()->begin(); it != SensorNet_System00->get_SensorMap()->end(); ++it) {
            
            ifstream decisionFile("/Users/LylaChen/Desktop/decisionTable.txt");
            for (int i = 1; i <= NO_TimeLevels*NO_EnergyLevels*NO_DataLevels*NO_DataLevels; i++) {
                
                int t, e, ld, cd;
                decisionFile >> t >> e >> ld >> cd >> (*(it->second->access_Decision_Table()))[t][e][ld][cd];
            }
            decisionFile.close();
            
        }
        
        Simulation(SensorNet_System00);
        
        delete SensorNet_System00;
    }
    
    
    return 0;
}


