#include <stdexcept>
#include <string>
#include "uvvm_cosim_data.hpp"
#include "vvc/vvc_factory.hpp"
#include "vvc/vvc_stream_based.hpp"
#include "vvc/vvc_mem_mapped.hpp"

namespace uvvm_cosim {

// ---------------------------------------------------------------------------
// Private map-access helpers (called from within a locked vvcInstanceMap lambda)
// ---------------------------------------------------------------------------

VvcBase* UvvmCosimData::findVvc(
    std::map<VvcIdentifier, std::unique_ptr<VvcBase>, VvcCompare>& m,
    const VvcIdentifier& id)
{
  auto it = m.find(id);
  if (it == m.end()) {
    throw std::runtime_error("VVC " + to_string(id) + " does not exist.");
  }
  return it->second.get();
}

const VvcBase* UvvmCosimData::findVvc(
    const std::map<VvcIdentifier, std::unique_ptr<VvcBase>, VvcCompare>& m,
    const VvcIdentifier& id)
{
  auto it = m.find(id);
  if (it == m.end()) {
    throw std::runtime_error("VVC " + to_string(id) + " does not exist.");
  }
  return it->second.get();
}

VvcStreamBased* UvvmCosimData::getStreamVvc(
    std::map<VvcIdentifier, std::unique_ptr<VvcBase>, VvcCompare>& m,
    const VvcIdentifier& id)
{
  auto* base = findVvc(m, id);
  auto* stream = dynamic_cast<VvcStreamBased*>(base);
  if (!stream) {
    throw std::runtime_error("VVC " + to_string(id) + " is not a stream-based VVC.");
  }
  return stream;
}

VvcMemMapped* UvvmCosimData::getMemVvc(
    std::map<VvcIdentifier, std::unique_ptr<VvcBase>, VvcCompare>& m,
    const VvcIdentifier& id)
{
  auto* base = findVvc(m, id);
  auto* mem = dynamic_cast<VvcMemMapped*>(base);
  if (!mem) {
    throw std::runtime_error("VVC " + to_string(id) + " is not a mem-mapped VVC.");
  }
  return mem;
}

// ---------------------------------------------------------------------------
// VVC registry
// ---------------------------------------------------------------------------

void UvvmCosimData::AddVvc(const std::string& vvc_type, const std::string& vvc_channel,
                            int vvc_instance_id,
                            const std::map<std::string, int>& bfm_cfg)
{
  std::unique_ptr<VvcBase> vvc;
  try {
    vvc = VvcFactory::create(vvc_type, vvc_channel, vvc_instance_id, bfm_cfg);
  } catch (const std::invalid_argument&) {
    // Unknown VVC type (e.g. CLOCK_GENERATOR_VVC) — skip silently
    return;
  }

  VvcIdentifier id(vvc->vvc_type, vvc->vvc_channel, vvc->vvc_instance_id);

  vvcInstanceMap([&](auto& m) {
    if (m.find(id) == m.end()) {
      m.emplace(id, std::move(vvc));
    } else {
      throw std::runtime_error("VVC " + to_string(id) + " already exists.");
    }
  });
}

std::vector<VvcInfo> UvvmCosimData::GetVvcList() const
{
  std::vector<VvcInfo> result;

  vvcInstanceMap([&](const auto& m) {
    for (const auto& [id, vvc] : m) {
      VvcInfo info;
      info.vvc_type        = to_string(vvc->vvc_type);
      info.vvc_channel     = to_string(vvc->vvc_channel);
      info.vvc_instance_id = vvc->vvc_instance_id;
      info.listen_enable   = vvc->listen_enable;
      info.cosim_support   = vvc->cosim_support;
      info.bfm_cfg         = vvc->bfm_cfg;

      // Extract packet_based from VvcStreamBased if applicable
      if (auto* s = dynamic_cast<VvcStreamBased*>(vvc.get())) {
        info.packet_based = s->packet_based;
      } else {
        info.packet_based = false;
      }

      result.push_back(std::move(info));
    }
  });

  return result;
}

void UvvmCosimData::SetVvcListenEnable(const VvcIdentifier& vvc, bool enable)
{
  vvcInstanceMap([&](auto& m) {
    findVvc(m, vvc)->listen_enable = enable;
  });
}

bool UvvmCosimData::GetVvcListenEnable(const VvcIdentifier& vvc) const
{
  return vvcInstanceMap([&](const auto& m) {
    return findVvc(m, vvc)->listen_enable;
  });
}

// ---------------------------------------------------------------------------
// Stream-based VVC queue API
// ---------------------------------------------------------------------------

bool UvvmCosimData::StreamQueueEmpty(QueueId qid, const VvcIdentifier& vvc)
{
  return vvcInstanceMap([&](auto& m) {
    return getStreamVvc(m, vvc)->QueueEmpty(qid);
  });
}

size_t UvvmCosimData::StreamQueueSize(QueueId qid, const VvcIdentifier& vvc)
{
  return vvcInstanceMap([&](auto& m) {
    return getStreamVvc(m, vvc)->QueueSize(qid);
  });
}

void UvvmCosimData::StreamQueuePutByte(QueueId qid, const VvcIdentifier& vvc,
                                        uint8_t byte, bool eop)
{
  vvcInstanceMap([&](auto& m) {
    getStreamVvc(m, vvc)->QueuePutByte(qid, byte, eop);
  });
}

void UvvmCosimData::StreamQueuePutPacket(QueueId qid, const VvcIdentifier& vvc,
                                          const std::vector<uint8_t>& pkt)
{
  vvcInstanceMap([&](auto& m) {
    getStreamVvc(m, vvc)->QueuePutPacket(qid, pkt);
  });
}

auto UvvmCosimData::StreamQueueGetByte(QueueId qid, const VvcIdentifier& vvc)
    -> std::optional<std::pair<uint8_t, bool>>
{
  return vvcInstanceMap([&](auto& m) {
    return getStreamVvc(m, vvc)->QueueGetByte(qid);
  });
}

auto UvvmCosimData::StreamQueueGetPacket(QueueId qid, const VvcIdentifier& vvc, int num_bytes)
    -> std::vector<uint8_t>
{
  return vvcInstanceMap([&](auto& m) {
    return getStreamVvc(m, vvc)->QueueGetPacket(qid, num_bytes);
  });
}

// ---------------------------------------------------------------------------
// Memory-mapped VVC queue API
// ---------------------------------------------------------------------------

void UvvmCosimData::MemQueuePut(QueueId qid, const VvcIdentifier& vvc, const Transfer& transfer)
{
  vvcInstanceMap([&](auto& m) {
    getMemVvc(m, vvc)->QueuePut(qid, transfer);
  });
}

auto UvvmCosimData::MemQueueGet(QueueId qid, const VvcIdentifier& vvc) -> std::optional<Transfer>
{
  return vvcInstanceMap([&](auto& m) {
    return getMemVvc(m, vvc)->QueueGet(qid);
  });
}

bool UvvmCosimData::MemQueueEmpty(QueueId qid, const VvcIdentifier& vvc)
{
  return vvcInstanceMap([&](auto& m) {
    return getMemVvc(m, vvc)->QueueEmpty(qid);
  });
}

} // namespace uvvm_cosim
