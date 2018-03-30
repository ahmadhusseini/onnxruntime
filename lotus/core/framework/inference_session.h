#ifndef LOTUS_CORE_FRAMEWORK_INFERENCE_SESSION_H_
#define LOTUS_CORE_FRAMEWORK_INFERENCE_SESSION_H_

#include "core/common/common.h"
#include "core/common/status.h"
#include "core/common/logging/logging.h"
#include "core/framework/execution_provider.h"
#include "core/framework/ml_value.h"
#include "core/platform/types.h"

namespace Lotus {
struct ProviderOption {
  ProviderOption(const std::string& provider_type0, const ExecutionProviderInfo& provider_info0)
      : provider_type(provider_type0),
        provider_info(provider_info0) {
  }
  std::string provider_type;
  ExecutionProviderInfo provider_info;
};

struct SessionOptions {
  // TODO what are the mandatory requirements for session options? and what should be the default
  // values for the remaining options? Tune this constructor appropriately once we learn more
  // about the answers to these questions.
  SessionOptions(const vector<ProviderOption>& ep_options0)
      : ep_options(ep_options0) {
  }
  //int num_threads; // not used now until we re-introduce threadpools for async execution
  vector<ProviderOption> ep_options;
  bool enable_sequential_execution = true;  // TODO: should we default to sequential execution?

  string session_logid;  ///< logger id to use for session output

  // TODO add more
};

struct RunOptions {
  bool enable_debug_mode = false;
  std::string run_tag = "";  // custom tag to tag all the runs
};

using NameMLValMap = std::unordered_map<std::string, MLValue>;

// Per model, handling multiple requests.
class InferenceSession {
 public:
  /**
     Create a new InferenceSession
     @param session_options Session options.
     @param logging_manager 
       Optional logging manager instance that will enable per session logger output using
       session_options.session_logid as the logger id in messages.
       If nullptr, the default LoggingManager MUST have been created previously as it will be used
       for logging. This will use the default logger id in messages.
       See core/common/logging/logging.h for details on how to do that, and how LoggingManager::DefaultLogger works.
     */
  explicit InferenceSession(const SessionOptions& session_options,
                            Logging::LoggingManager* logging_manager = nullptr);
  ~InferenceSession();

  // Load an ONNX model and initialize.
  Common::Status Load(const std::string& model_uri);

  //
  Common::Status Initialize();

  // Both feeds and fetches are owned by client code, and can't be changed
  // by client code during Run().
  Common::Status Run(const NameMLValMap& feeds,
                     const std::vector<std::string>& output_names,
                     std::vector<MLValue>* p_fetches);

  Common::Status Run(const RunOptions& run_options,
                     const NameMLValMap& feeds,
                     const std::vector<std::string>& output_names,
                     std::vector<MLValue>* p_fetches);

  // Get current num threads running Run
  int GetCurrentNumRuns();

 private:
  class Impl;
  std::unique_ptr<Impl> impl_;
  LOTUS_DISALLOW_COPY_ASSIGN_AND_MOVE(InferenceSession);
};
}  // namespace Lotus

#endif  // LOTUS_CORE_FRAMEWORK_INFERENCE_SESSION_H_
