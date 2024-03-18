#include "Timing.hpp"
#include "darknet_internal.hpp"


namespace
{
	Darknet::TimingRecords tr;
	static pthread_mutex_t timing_and_tracking_container_mutex = PTHREAD_MUTEX_INITIALIZER;
}


Darknet::TimingAndTracking::TimingAndTracking(const std::string& n)
{
	name = n;
	start_time = std::chrono::high_resolution_clock::now();

	return;
}


Darknet::TimingAndTracking::~TimingAndTracking()
{
	end_time = std::chrono::high_resolution_clock::now();

	tr.add(*this);

	return;
}


Darknet::TimingRecords::TimingRecords()
{
	return;
}


Darknet::TimingRecords::~TimingRecords()
{
	#ifdef DARKNET_TIMING_AND_TRACKING_ENABLED

	const VStr cols =
	{
		"calls",
		"min",
		"max",
		"total",
		"average",
		"function"
	};
	const MStrInt m =
	{
		{"calls"	, 8},
		{"min"		, 8},
		{"max"		, 8},
		{"total"	, 12},
		{"average"	, 8},
		{"function"	, 8},
	};

	std::string seperator;
	for (const auto & name : cols)
	{
		const int len = m.at(name);
		seperator += "+-" + std::string(len, '-') + "-";
	}
	std::cout << seperator << std::endl;
	for (const auto & name : cols)
	{
		std::cout << "| " << std::setw(m.at(name)) << name << " ";
	}
	std::cout << std::endl << seperator << std::endl;

	size_t skipped = 0;
	for (const auto & [k, v] : number_of_calls_per_function)
	{
		const auto & name = k;
		const auto & calls = v;
		const auto & total_milliseconds = total_elapsed_time_per_function.at(name);
		const auto & min_milliseconds = min_elapsed_time_per_function.at(name);
		const auto & max_milliseconds = max_elapsed_time_per_function.at(name);
		const auto average_milliseconds = float(total_milliseconds) / float(calls);

		if (total_milliseconds < 1000.0f)
		{
			skipped ++;
			continue;
		}

		auto display_name = name.substr(0, 100);
		if (name.size() > 100)
		{
			display_name += "...";
		}
		std::cout
			<< "| " << std::setw(m.at("calls")) << calls << " "
			<< "| " << std::setw(m.at("min")) << min_milliseconds << " "
			<< "| " << std::setw(m.at("max")) << max_milliseconds << " "
			<< "| " << std::setw(m.at("total")) << total_milliseconds << " "
			<< "| " << std::setw(m.at("average")) << std::fixed << std::setprecision(1) << average_milliseconds << " "
			<< "| " << display_name
			<< std::endl;
	}
	std::cout
		<< seperator << std::endl
		<< "Entries skipped:  " << skipped << std::endl;

	#endif

	return;
}


Darknet::TimingRecords & Darknet::TimingRecords::add(const Darknet::TimingAndTracking & tat)
{
	#ifdef DARKNET_TIMING_AND_TRACKING_ENABLED

	const auto duration = tat.end_time - tat.start_time;
	const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

	timespec ts;
	ts.tv_nsec = 0;
	ts.tv_sec = std::time(nullptr) + 5;
	const int rc = pthread_mutex_timedlock(&timing_and_tracking_container_mutex, &ts);

	number_of_calls_per_function[tat.name] ++;
	total_elapsed_time_per_function[tat.name] += milliseconds;

	if (min_elapsed_time_per_function.count(tat.name) == 0 or milliseconds < min_elapsed_time_per_function[tat.name])
	{
		min_elapsed_time_per_function[tat.name] = milliseconds;
	}
	if (max_elapsed_time_per_function.count(tat.name) == 0 or milliseconds > max_elapsed_time_per_function[tat.name])
	{
		max_elapsed_time_per_function[tat.name] = milliseconds;
	}

	if (rc == 0)
	{
		pthread_mutex_unlock(&timing_and_tracking_container_mutex);
	}

	#endif

	return *this;
}
