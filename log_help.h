#pragma once

#define TRACE_FUNCTION BOOST_LOG_TRIVIAL(trace) << "entering: " << __FUNCSIG__;