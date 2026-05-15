#pragma once
#include <atomic>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include "shared_map.hpp"
#include "uvvm_cosim_types.hpp"
#include "vvc/vvc_stream_based.hpp"
#include "vvc/vvc_mem_mapped.hpp"

namespace uvvm_cosim {

using VvcMap = shared_map<VvcIdentifier, std::unique_ptr<VvcBase>, VvcCompare>;

class UvvmCosimData {
private:
  VvcMap vvcInstanceMap;

  std::atomic<bool> startSim     = false;
  std::atomic<bool> terminateSim = false;

  // Returns the VVC for the given identifier; throws if not found.
  // Must be called from within a vvcInstanceMap lambda (map is already locked).
  static VvcBase* findVvc(std::map<VvcIdentifier, std::unique_ptr<VvcBase>, VvcCompare>& m,
                          const VvcIdentifier& id);

  // As above but for const map access.
  static const VvcBase* findVvc(const std::map<VvcIdentifier, std::unique_ptr<VvcBase>, VvcCompare>& m,
                                const VvcIdentifier& id);

  // Downcasts to VvcStreamBased; throws if VVC does not exist or is not stream-based.
  static VvcStreamBased* getStreamVvc(std::map<VvcIdentifier, std::unique_ptr<VvcBase>, VvcCompare>& m,
                                      const VvcIdentifier& id);

  // Downcasts to VvcMemMapped; throws if VVC does not exist or is not mem-mapped.
  static VvcMemMapped* getMemVvc(std::map<VvcIdentifier, std::unique_ptr<VvcBase>, VvcCompare>& m,
                                 const VvcIdentifier& id);

public:
  UvvmCosimData()  = default;
  ~UvvmCosimData() = default;

  /////////////////////////////////////////////////////////////////////////////
  // Simulation control flags
  /////////////////////////////////////////////////////////////////////////////

  void setStartSim(bool value)    { startSim     = value; }
  bool getStartSim()    const     { return startSim;      }
  void setTerminateSim(bool value){ terminateSim = value; }
  bool getTerminateSim()const     { return terminateSim;  }

  /////////////////////////////////////////////////////////////////////////////
  // VVC registry
  /////////////////////////////////////////////////////////////////////////////

  // Parse bfm_cfg string and create the appropriate VVC object via VvcFactory.
  // Unknown VVC types (e.g. CLOCK_GENERATOR_VVC) are silently ignored.
  void AddVvc(const std::string& vvc_type, const std::string& vvc_channel,
              int vvc_instance_id, const std::map<std::string, int>& bfm_cfg);

  std::vector<VvcInfo> GetVvcList() const;

  void SetVvcListenEnable(const VvcIdentifier& vvc, bool enable);
  bool GetVvcListenEnable(const VvcIdentifier& vvc) const;

  /////////////////////////////////////////////////////////////////////////////
  // Stream-based VVC queue API
  // Both byte-stream and packet-stream VVCs share this interface via virtual dispatch.
  /////////////////////////////////////////////////////////////////////////////

  bool   StreamQueueEmpty(QueueId qid, const VvcIdentifier& vvc);
  size_t StreamQueueSize(QueueId qid, const VvcIdentifier& vvc);

  void   StreamQueuePutByte(QueueId qid, const VvcIdentifier& vvc, uint8_t byte, bool eop);
  void   StreamQueuePutPacket(QueueId qid, const VvcIdentifier& vvc, const std::vector<uint8_t>& pkt);

  // Get next byte: eop is always false for byte-stream VVCs.
  auto   StreamQueueGetByte(QueueId qid, const VvcIdentifier& vvc)
           -> std::optional<std::pair<uint8_t, bool>>;

  // For byte-stream VVCs returns up to num_bytes bytes.
  // For packet-stream VVCs returns the next complete packet (num_bytes ignored).
  auto   StreamQueueGetPacket(QueueId qid, const VvcIdentifier& vvc, int num_bytes)
           -> std::vector<uint8_t>;

  /////////////////////////////////////////////////////////////////////////////
  // Memory-mapped VVC queue API
  /////////////////////////////////////////////////////////////////////////////

  void MemQueuePut(QueueId qid, const VvcIdentifier& vvc, const Transfer& transfer);
  auto MemQueueGet(QueueId qid, const VvcIdentifier& vvc) -> std::optional<Transfer>;
  bool MemQueueEmpty(QueueId qid, const VvcIdentifier& vvc);
};

} // namespace uvvm_cosim
