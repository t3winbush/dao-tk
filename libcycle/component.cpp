#include "component.h"
#include <cmath>
#include <algorithm>
#include <iostream>


//##################################################################################

failure_event::failure_event(){}

failure_event::failure_event(int _time, std::string _component, int _fail_idx, double _duration, double _new_life)
    : time( _time ), component( _component ), fail_idx( _fail_idx), duration( _duration ), new_life( _new_life )
{}


std::string failure_event::print()
{
    return "<" + component + ", " + std::to_string(duration) + ", " + std::to_string(new_life);
}

//##################################################################################

ComponentStatus::ComponentStatus(){}

ComponentStatus::ComponentStatus(std::vector<double> lifes, double hazard, double downtime )
    : lifetimes(lifes), hazard_rate( hazard ), downtime_remaining( downtime )
{}

//##################################################################################

Component::Component()
{
}

Component::Component(  std::string name, std::string type, 
			//std::string dist_type, double failure_alpha, double failure_beta, 
			double repair_rate, double repair_cooldown_time,
            double hot_start_penalty, double warm_start_penalty, 
			double cold_start_penalty, 
            std::unordered_map< std::string, failure_event > *failure_events, 
			double availability_reduction, double repair_cost)
{
    /*
    Description of attributes:
    name -- component identifer
    component_type -- component type description
	dist_type -- failure distribution type (Gamma or Beta)  Note:  Exponential is Gamma(1, lambda)
    failure_alpha -- alpha for Gamma or Beta distribution
	failure_beta -- beta for Gamma or Beta distribution
    repair_rate -- rate at which repairs take place (events/h)
    repair_cooldown_time -- added required downtime for repair (h)
    repair_cost -- dollar cost of repairs, not including revenue lost ($)
    hot_start_penalty -- increase in hazard rate due to hot start
    warm_start_penalty -- increase in hazard rate due to warm start
    cold_start_penalty -- increase in hazard rate due to cold start
    */

    if( name == "MAINTENANCE" )
        throw std::exception("cannot name a component 'MAINTENANCE'");
    
	m_failure_types = {};
    m_name = name;
    m_type = type;
    m_hot_start_penalty = hot_start_penalty;
    m_warm_start_penalty = warm_start_penalty;
    m_cold_start_penalty = cold_start_penalty;
    m_repair_cost = repair_cost;
	m_availability_reduction = availability_reduction;
	m_cooldown_time = repair_cooldown_time;

	m_status.hazard_rate = 1.0;
    m_status.downtime_remaining = 0.0;
	m_status.operational = true;

	Distribution *edist = new ExponentialDist(repair_rate, repair_cooldown_time, "exponential");
	m_repair_dist = (ExponentialDist *) edist;

    m_parent_failure_events = failure_events;
}


void Component::ReadStatus( ComponentStatus &status )
{
	m_status.hazard_rate =  status.hazard_rate;
    m_status.downtime_remaining = status.downtime_remaining;
	m_status.operational = (m_status.downtime_remaining < 1e-8);
	for (size_t j = 0; j < m_failure_types.size(); j++)
	{
		m_failure_types.at(j).SetLifeOrProb(status.lifetimes.at(j));
	}
}

        
std::string Component::GetName()
{
	return m_name;
}

        
std::string Component::GetType()
{
	return m_type;
}

std::vector<FailureType> Component::GetFailureTypes()
{
	return m_failure_types;
}

void Component::AddFailureMode(std::string component, std::string id, std::string failure_mode,
	std::string dist_type, double alpha, double beta)
{
	m_failure_types.push_back(FailureType(component, id, failure_mode, dist_type, alpha, beta));
}
        
double Component::GetHazardRate()
{
	return m_status.hazard_rate;
}

    
double Component::GetRepairCost()
{
	return m_repair_cost;
}

double Component::GetAvailabilityReduction()
{
	return m_availability_reduction;
}

double Component::GetCooldownTime()
{
	return m_cooldown_time;
}

        
bool Component::IsOperational()
{
	return m_status.operational;
}

        
void Component::Shutdown(double time)
{
    /*
    Removes component from operation for a given period of time 
    (used for maintenance).
    time -- required downtime for maintenance
    retval -- None 
    */
	m_status.operational = false;
    m_status.downtime_remaining = time;
}

        
void Component::RestoreComponent()
{
	m_status.operational = true;
	m_status.downtime_remaining = 0.0;
}

        
void Component::ResetHazardRate()
{
	m_status.hazard_rate = 1.;
}

    
double Component::GetDowntimeRemaining()
{
	return m_status.downtime_remaining;
}

        
void Component::SetDowntimeRemaining(double time)
{
	m_status.downtime_remaining = time;
}


double Component::GetHotStartPenalty()
{
	return m_hot_start_penalty;
}


double Component::GetWarmStartPenalty()
{
	return m_warm_start_penalty;
}


double Component::GetColdStartPenalty()
{
	return m_cold_start_penalty;
}

        
void Component::GenerateTimeToRepair(WELLFiveTwelve &gen)
{
	/* 
    generate a random downtime, which is the number of hours of
    operation to a component failure given normal operations that do not
    increase the hazard rate.
    gen -- random U[0,1] generator object
    retval -- lifetime in adjusted hours of operation
    */
    m_status.downtime_remaining = m_repair_dist->GetVariate(gen);
	//std::cerr << "NEW FAILURE - DOWNTIME: " << std::to_string(m_status.downtime_remaining) << "\n";
}

        
double Component::HoursToFailure(double ramp_mult, std::string mode)
{
    /* 
    Returns number of hours of operation that would lead to failure 
    under the operation parameters given as input.
        
    ramp_mult -- degradation multiplier due to ramping
	mode -- operating mode
    retval - floating point number indicating hours of operation to failure
    */
    if( ramp_mult == 0 )
        return INFINITY;
	double hours = INFINITY;
	for (size_t j = 0; j < m_failure_types.size(); j++)
	{
		if (m_failure_types.at(j).GetFailureMode() == mode)
		{
			hours = std::min(hours, 
				m_failure_types.at(j).GetLifeRemaining() / 
				(m_status.hazard_rate * ramp_mult)
				);
		}
	}
    return INFINITY;
}


void Component::TestForBinaryFailure(std::string mode, int t, WELLFiveTwelve &gen)
{
	double var = 0.0;
	for (size_t j = 0; j < m_failure_types.size(); j++)
	{
		if (m_failure_types.at(j).GetFailureMode() == mode)
		{
			var = gen.getVariate();
			if (var*m_status.hazard_rate <= m_failure_types.at(j).GetFailureProbability())
				GenerateFailure(gen, t, j);
		}
	}
}

void Component::TestForFailure(double time, double ramp_mult, WELLFiveTwelve &gen, int t, std::string start, std::string mode)
{
	/*
	Generates failure events under the provided dispatch, if there is not sufficient life
	remaining in the component, or the RNG generates a failure on start.
	*/
	double hazard_mult = 0.0;
	if (start == "HotStart")
		hazard_mult = m_hot_start_penalty;
	if (start == "WarmStart")
		hazard_mult = m_warm_start_penalty;
	if (start == "ColdStart")
		hazard_mult = m_cold_start_penalty;
	std::string opmode;
	if (mode == "OS")
	// if starting standby or online, test for fail on start, 
	// then operate as if in the first hour of that mode to test
	// for failures during the time period.
	{
		TestForBinaryFailure(mode, t, gen);
		opmode = "OF";
	}
	else if (mode == "SS")
	{
		TestForBinaryFailure(mode, t, gen);
		opmode = "SF";
	}
	else
		opmode = mode;
	for (size_t j = 0; j < m_failure_types.size(); j++)
	{
		if (m_failure_types.at(j).GetFailureMode() == opmode)
		{
			if (time * (m_status.hazard_rate + hazard_mult) * ramp_mult > m_failure_types.at(j).GetLifeRemaining())
				GenerateFailure(gen, t, j);
		}
	}
	
}

         
void Component::Operate(double time, double ramp_mult, WELLFiveTwelve &gen, 
		bool read_only, int t, std::string start, std::string mode)
{
    /* 
    assumes operation for a given period of time, with 
    no permanent change to the hazard rate.
    time -- time of operation
    ramp_mult -- degradation multiplier due to ramping
    gen -- random U[0,1] variate generator object
    read_only -- indicates whether to generate a failure event if 
        life_remaining falls below 0 during operation
    failure_file -- output file to record failures
    t -- period index
    retval -- None
    */
    if( ! IsOperational() )
        throw std::exception("can't operate a plant in downtime.");
	if( start == "HotStart")
		m_status.hazard_rate += m_hot_start_penalty;
	if (start == "WarmStart")
		m_status.hazard_rate += m_warm_start_penalty;
	if (start == "ColdStart")
		m_status.hazard_rate += m_cold_start_penalty;
	std::string opmode;
	//if starting a mode, operate as if
	//in the first hour of operation for that mode to degrade lifetimes.
	if (mode == "OS")
		// if starting standby or online, test for fail on start, 
		// then operate as if in the first hour of that mode to test
		// for failures during the time period.
	{
		TestForBinaryFailure(mode, t, gen);
		opmode = "OF";
	}
	else if (mode == "SS")
	{
		TestForBinaryFailure(mode, t, gen);
		opmode = "SF";
	}
	else
		opmode = mode;
	for (size_t j = 0; j < m_failure_types.size(); j++)
	{
		if (m_failure_types.at(j).GetFailureMode() == opmode)
		{
			if (time * m_status.hazard_rate * ramp_mult > m_failure_types.at(j).GetLifeRemaining() && !read_only)
				throw std::exception("failure should be thrown.");
			m_failure_types.at(j).ReduceLifeRemaining(time * m_status.hazard_rate * ramp_mult);
		}
	}		
}
         
void Component::ReadFailure(double downtime, double life_remaining, 
	int fail_idx, bool reset_hazard)
{
    /*
    reads a failure event.  This executes the failure without the 
    randomly generated failures.
    downtime -- downtime to apply to the failure
    life_remaining -- operational lifetime of component once online again
	rest_hazard -- true if the repair resets the hazard rate, false o.w.
    retval -- none
    */
	m_status.operational = false;
    SetDowntimeRemaining(downtime);
    m_failure_types.at(fail_idx).SetLifeOrProb(life_remaining);
    ResetHazardRate();
}

                
void Component::GenerateFailure(WELLFiveTwelve &gen, int t, int fail_idx)
{
    /*
    creates a failure event, shutting down the plant for a period of time.
    gen -- random U[0,1] generator used to generate stochastic repair times
    failure_file -- failure event output file
    t -- period index
	fail_idx -- failure type (mode/part combination) that caused failure
    retval -- None
    */
    m_status.operational = false;
    GenerateTimeToRepair(gen);
    m_failure_types.at(fail_idx).GenerateFailureVariate(gen);
    ResetHazardRate();
    
    //add a new failure to the parent (CSPPlant) failure queue
	std::string label = std::to_string(t)+GetName()+std::to_string(fail_idx);
    (*m_parent_failure_events)[label] = failure_event(
		t, GetName(), fail_idx, m_status.downtime_remaining, 
		m_failure_types.at(fail_idx).GetLifeOrProb()
		);
	//std::cerr << "FAILURE EVENT GENERATED. downtime: " << std::to_string(m_status.downtime_remaining) << " life_rem: " << std::to_string(m_failure_types.at(fail_idx).GetLifeOrProb()) << " fail idx: " << std::to_string(fail_idx) << " reset hazard rate: " << std::to_string(true) << "\n";

}
 
        
void Component::AdvanceDowntime(double time)
{
    //moves forward in time while the plant is down.
    m_status.downtime_remaining -= time;
    if( m_status.downtime_remaining <= 0.0 )
    {
        m_status.downtime_remaining = 0.0;
        m_status.operational = true;
    }
}

std::vector<double> Component::GetLifetimesAndProbs()
{
	//returns a vector of component lifetimes/probabilities.
	std::vector<double> lifetimes_and_probs = {};
	for (size_t j=0; j < m_failure_types.size(); j++)
	{
		lifetimes_and_probs.push_back(m_failure_types.at(j).GetLifeOrProb());
	}
	return lifetimes_and_probs;
}
            
void Component::GenerateInitialLifesAndProbs(WELLFiveTwelve &gen)
{
	/*
	generates initial failures and probabilities for a component as if it were
	brand new.
	*/
	for (std::vector<FailureType>::iterator it = m_failure_types.begin(); 
		it != m_failure_types.end(); it++)
	{
		it->GenerateFailureVariate(gen);
	}
}

ComponentStatus Component::GetState()
{
    //returns current state as a ComponentStatus.
	return (
		ComponentStatus(
			GetLifetimesAndProbs(), m_status.hazard_rate*1.0,
			m_status.downtime_remaining*1.0
			)
		); 
}