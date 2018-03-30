#pragma once
// NOTE: Don't include this file directly. Include logging.h

#define CREATE_MESSAGE(logger, severity, category, datatype) \
  Lotus::Logging::Capture(logger, Lotus::Logging::Severity::k##severity, category, datatype, WHERE)

/*
Both printf and stream style logging are supported.
Not that printf currently has a 2K limit to the message size.

LOGS_* macros are for stream style
LOGF_* macros are for printf style

The Message class captures the log input, and pushes it through the logger in its destructor.

Use the *FATAL* macros if you want a Severity::kFatal message to also throw.

There are a few variants to minimize the length of the macro name required in the calling code. 
They are optimized so the shortest names are for the (expected) most common usage. This can be
tweaked if needed.

Explicit logger vs LoggingManager::DefaulLogger()
  Default is for a logger instance to be explicitly passed in. 
  The logger instance provides an identifier so that log messages from different runs can be separated.

  Variants with DEFAULT in the macro name use the default logger provided by logging manager. This is
  static so accessible from any code, provided a LoggingManager instance created with InstanceType::Default 
  exists somewhere. See logging.h for further explanation of the expected setup.
  
DataType
  Default uses DataType::SYSTEM.
  
  Variants with USER in the macro name use DataType::USER. This is data that could be PII, and may need to
  be filtered from output. LoggingManager applies this filtering.

Category
  Default category is Lotus::Logging::Category::Lotus.

  If you wish to provide a different category, use variants with CATEGORY in the macro name

*/

// Logging with explicit category

// iostream style logging. Capture log info in Message, and push to the logger in ~Message.
#define LOGS_CATEGORY(logger, severity, category)                                                        \
  if ((logger).OutputIsEnabled(Lotus::Logging::Severity::k##severity, Lotus::Logging::DataType::SYSTEM)) \
  CREATE_MESSAGE(logger, severity, category, Lotus::Logging::DataType::SYSTEM).Stream()

#define LOGS_USER_CATEGORY(logger, severity, category)                                                 \
  if ((logger).OutputIsEnabled(Lotus::Logging::Severity::k##severity, Lotus::Logging::DataType::USER)) \
  CREATE_MESSAGE(logger, severity, category, Lotus::Logging::DataType::USER).Stream()

// printf style logging. Capture log info in Message, and push to the logger in ~Message.
#define LOGF_CATEGORY(logger, severity, category, format_str, ...)                                       \
  if ((logger).OutputIsEnabled(Lotus::Logging::Severity::k##severity, Lotus::Logging::DataType::SYSTEM)) \
  CREATE_MESSAGE(logger, severity, category, Lotus::Logging::DataType::SYSTEM).CapturePrintf(format_str, ##__VA_ARGS__)

#define LOGF_USER_CATEGORY(logger, severity, category, format_str, ...)                                \
  if ((logger).OutputIsEnabled(Lotus::Logging::Severity::k##severity, Lotus::Logging::DataType::USER)) \
  CREATE_MESSAGE(logger, severity, category, Lotus::Logging::DataType::USER).CapturePrintf(format_str, ##__VA_ARGS__)

// Logging with category of "Lotus"

#define LOGS(logger, severity) \
  LOGS_CATEGORY(logger, severity, Lotus::Logging::Category::Lotus)

#define LOGS_USER(logger, severity) \
  LOGS_USER_CATEGORY(logger, severity, Lotus::Logging::Category::Lotus)

// printf style logging. Capture log info in Message, and push to the logger in ~Message.
#define LOGF(logger, severity, format_str, ...) \
  LOGF_CATEGORY(logger, severity, Lotus::Logging::Category::Lotus, format_str, ##__VA_ARGS__)

#define LOGF_USER(logger, severity, format_str, ...) \
  LOGF_USER_CATEGORY(logger, severity, Lotus::Logging::Category::Lotus, format_str, ##__VA_ARGS__)

/*

Macros that use the default logger. 
A LoggingManager instance must be currently valid for the default logger to be available.

*/

// Logging with explicit category

#define LOGS_DEFAULT_CATEGORY(severity, category) \
  LOGS_CATEGORY(Lotus::Logging::LoggingManager::DefaultLogger(), severity, category)

#define LOGS_USER_DEFAULT_CATEGORY(severity, category) \
  LOGS_USER_CATEGORY(Lotus::Logging::LoggingManager::DefaultLogger(), severity, category)

#define LOGF_DEFAULT_CATEGORY(severity, category, format_str, ...) \
  LOGF_CATEGORY(Lotus::Logging::LoggingManager::DefaultLogger(), severity, category, format_str, ##__VA_ARGS__)

#define LOGF_USER_DEFAULT_CATEGORY(severity, category, format_str, ...) \
  LOGF_USER_CATEGORY(Lotus::Logging::LoggingManager::DefaultLogger(), severity, category, format_str, ##__VA_ARGS__)

// Logging with category of "Lotus"

#define LOGS_DEFAULT(severity) \
  LOGS_DEFAULT_CATEGORY(severity, Lotus::Logging::Category::Lotus)

#define LOGS_USER_DEFAULT(severity) \
  LOGS_USER_DEFAULT_CATEGORY(severity, Lotus::Logging::Category::Lotus)

#define LOGF_DEFAULT(severity, format_str, ...) \
  LOGF_DEFAULT_CATEGORY(severity, Lotus::Logging::Category::Lotus, format_str, ##__VA_ARGS__)

#define LOGF_USER_DEFAULT(severity, format_str, ...) \
  LOGF_USER_DEFAULT_CATEGORY(severity, Lotus::Logging::Category::Lotus, format_str, ##__VA_ARGS__)

/*

Conditional logging

*/

// Logging with explicit category

#define LOGS_CATEGORY_IF(boolean_expression, logger, severity, category) \
  if ((boolean_expression) == true) LOGS_CATEGORY(logger, severity, category)

#define LOGS_DEFAULT_CATEGORY_IF(boolean_expression, severity, category) \
  if ((boolean_expression) == true) LOGS_DEFAULT_CATEGORY(severity, category)

#define LOGS_USER_CATEGORY_IF(boolean_expression, logger, severity, category) \
  if ((boolean_expression) == true) LOGS_USER_CATEGORY(logger, severity, category)

#define LOGS_USER_DEFAULT_CATEGORY_IF(boolean_expression, severity, category) \
  if ((boolean_expression) == true) LOGS_USER_DEFAULT_CATEGORY(severity, category)

#define LOGF_CATEGORY_IF(boolean_expression, logger, severity, category, format_str, ...) \
  if ((boolean_expression) == true) LOGF_CATEGORY(logger, severity, category, format_str, ##__VA_ARGS__)

#define LOGF_DEFAULT_CATEGORY_IF(boolean_expression, severity, category, format_str, ...) \
  if ((boolean_expression) == true) LOGF_DEFAULT_CATEGORY(severity, category, format_str, ##__VA_ARGS__)

#define LOGF_USER_CATEGORY_IF(boolean_expression, logger, severity, category, format_str, ...) \
  if ((boolean_expression) == true) LOGF_USER_CATEGORY(logger, severity, category, format_str, ##__VA_ARGS__)

#define LOGF_USER_DEFAULT_CATEGORY_IF(boolean_expression, severity, category, format_str, ...) \
  if ((boolean_expression) == true) LOGF_USER_DEFAULT_CATEGORY(severity, category, format_str, ##__VA_ARGS__)

// Logging with category of "Lotus"

#define LOGS_IF(boolean_expression, logger, severity) \
  LOGS_CATEGORY_IF(boolean_expression, logger, severity, Lotus::Logging::Category::Lotus)

#define LOGS_DEFAULT_IF(boolean_expression, severity) \
  LOGS_DEFAULT_CATEGORY_IF(boolean_expression, severity, Lotus::Logging::Category::Lotus)

#define LOGS_USER_IF(boolean_expression, logger, severity) \
  LOGS_USER_CATEGORY_IF(boolean_expression, logger, severity, Lotus::Logging::Category::Lotus)

#define LOGS_USER_DEFAULT_IF(boolean_expression, severity) \
  LOGS_USER_DEFAULT_CATEGORY_IF(boolean_expression, severity, Lotus::Logging::Category::Lotus)

#define LOGF_IF(boolean_expression, logger, severity, format_str, ...) \
  LOGF_CATEGORY_IF(boolean_expression, logger, severity, Lotus::Logging::Category::Lotus, format_str, ##__VA_ARGS__)

#define LOGF_DEFAULT_IF(boolean_expression, severity, format_str, ...) \
  LOGF_DEFAULT_CATEGORY_IF(boolean_expression, severity, Lotus::Logging::Category::Lotus, format_str, ##__VA_ARGS__)

#define LOGF_USER_IF(boolean_expression, logger, severity, format_str, ...) \
  LOGF_USER_CATEGORY_IF(boolean_expression, logger, severity, Lotus::Logging::Category::Lotus, format_str, ##__VA_ARGS__)

#define LOGF_USER_DEFAULT_IF(boolean_expression, severity, format_str, ...) \
  LOGF_USER_DEFAULT_CATEGORY_IF(boolean_expression, severity, Lotus::Logging::Category::Lotus, format_str, ##__VA_ARGS__)

/*

Debug verbose logging of caller provided level.
Disabled in Release builds.
Use the _USER variants for VLOG statements involving user data that may need to be filtered.
*/
#define VLOGS(logger, level) \
  if (level < Lotus::Logging::max_vlog_level) LOGS_CATEGORY(logger, VERBOSE, "VLOG" #level)

#define VLOGS_USER(logger, level) \
  if (level < Lotus::Logging::max_vlog_level) LOGS_USER_CATEGORY(logger, VERBOSE, "VLOG" #level)

#define VLOGF(logger, level, format_str, ...) \
  if (level < Lotus::Logging::max_vlog_level) LOGF_CATEGORY(logger, VERBOSE, "VLOG" #level, format_str, ##__VA_ARGS__)

#define VLOGF_USER(logger, level, format_str, ...) \
  if (level < Lotus::Logging::max_vlog_level) LOGF_USER_CATEGORY(logger, VERBOSE, "VLOG" #level, format_str, ##__VA_ARGS__)

// Default logger variants
#define VLOGS_DEFAULT(level) \
  VLOGS(Lotus::Logging::LoggingManager::DefaultLogger(), level)

#define VLOGS_USER_DEFAULT(level) \
  VLOGS_USER(Lotus::Logging::LoggingManager::DefaultLogger(), level)

#define VLOGF_DEFAULT(level, format_str, ...) \
  VLOGF(Lotus::Logging::LoggingManager::DefaultLogger(), level, format_str, ##__VA_ARGS__)

#define VLOGF_USER_DEFAULT(level, format_str, ...) \
  VLOGF_USER(Lotus::Logging::LoggingManager::DefaultLogger(), level, format_str, ##__VA_ARGS__)

/***
Disabling in favor of LOTUS_THROW and LOTUS_ENFORCE (throws exception, relies on something else to log).
Leaving here in case we decide we want macros to provide log first, throw second behavior.
// TODO: Consider alternative.
// The below *FATAL* macros are more testable in that they throw via LogFatalAndThrow.
// Alternatively, LoggingManager can throw after sending a Severity::kFatal message
// as that happens in the context of Capture destructor, leading to std::terminate being
// called. That behavior is more consistent (using kFatal with any log statement will exit)
// however possibly harder to debug, especially if the log message with the fatal error
// hasn't been flushed to the sink when the terminate occurs. Throwing here and catching in a
// top level handler has cleaner unwind semantics, even if that simply re-throws.

// Log at Severity::Fatal and throw.
#define FATAL(category, format_str, ...) \
  throw Lotus::Logging::LoggingManager::LogFatalAndCreateException(category, WHERE_WITH_STACK, format_str, ##__VA_ARGS__)

// If condition is true, log the condition at Severity::Fatal and throw
#define FATAL_IF(boolean_expression) \
  if (boolean_expression)            \
  FATAL(Lotus::Logging::Category::Lotus, #boolean_expression)

* 
Check macros
*

// The CHECK* macros are of questionable value, given we log the condition in FATAL_IF anyway.
// e.g. using CHECK_EQ(a, b) is not that different to FATAL_IF(a != b), and using FATAL_IF
// everywhere is more consistent and easier for a new developer to know what to do.

#define CHECK_NOTNULL(ptr) \
  FATAL_IF((ptr) == nullptr)

// kFatal if the check does not pass
#define CHECK_OP(val1, val2, op) \
  FATAL_IF(!(val1 op val2))
#define CHECK(val) CHECK_OP(val, true, ==)
#define CHECK_EQ(val1, val2) CHECK_OP(val1, val2, ==)
#define CHECK_NE(val1, val2) CHECK_OP(val1, val2, !=)
#define CHECK_LE(val1, val2) CHECK_OP(val1, val2, <=)
#define CHECK_LT(val1, val2) CHECK_OP(val1, val2, <)
#define CHECK_GE(val1, val2) CHECK_OP(val1, val2, >=)
#define CHECK_GT(val1, val2) CHECK_OP(val1, val2, >)

*/
