#ifndef ICTCP_H
#define ICTCP_H

#include "ns3/tcp-congestion-ops.h"
#include "ns3/tcp-recovery-ops.h"

namespace ns3 {

/**
 * \ingroup congestionOps
 *
 * \brief An implementation of ICTCP
 *
 * Firstly, it measures the total incoming traffic on the network interface as BWT,
 * compute the available bandwidth as BWA=max(0,αC−BWT), where α∈[0,1] is a parameter to absorb oversubscription.
 * In the paper, α=0.9.
 *
 * Secondly, there is a global time-keeping to divide the time into slots of 2T, and each slot is divided into two subslots of T.
 * The first slot is for measurement of BWA, and second slot is for increasing rwnd.
 * The increase is per-connection, but the total increase across all connections is guided by BWA.
 * Each connection would have its own RTT.
 * The rwnd on each connection can be increased only when now it is in the second subslot and the last increase for this connection is more than 2RTT ago.
 * The size of a subslot is determined by T=∑iwiRTTi where wi is the normalized traffic volume of connection i over all traffic.
 *
 * Then, the window adjustment is as follows:
 * For every RTT on connection i, we sample its current throughput (bytes received over RTT) as bsi.
 * Then a measured throughput is calculated as EWMA:
 * (bmi,new)=max(bsi,βbmi,old+(1−β)bsi).
 *
 * The max is to update the measured throughput as soon as rwnd is increased.
 * Then the expected throughput for this connection is bei=max(bmi,rwndi/RTTi).
 * The max is to ensure bmi≤bei. Now define the throughput difference ratio as dbi=(bei−bmi)/bei, which is between 0 and 1.
 * Then the rwnd is adjusted according to this difference ratio:
 * If dbi≤γ1 or smaller than MSS/rwnd, increase the rwnd if it is in the second subslot and there are enough quota.
 * If dbi>γ2 for three continuous RTT, decrease rwnd by one MSS
 *
 * All other cases, keep the rwnd
 * The paper suggested γ1=0.1 and γ2=0.5 in the experiments.
 * Increase is triggered when the throughput difference is less than one MSS.
 * Decrease is restricted to one MSS per RTT to prevent the sliding window have to shift backward.
 */

class IcTcp : public TcpNewReno
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  /**
   * Create an unbound tcp socket.
   */
  IcTcp (void);

  /**
   * \brief Copy constructor
   * \param sock the object to copy
   */
  IcTcp (const IcTcp& sock);
  virtual ~IcTcp (void);

  virtual std::string GetName () const;

  /**
   * \brief Compute RTTs needed to execute Vegas algorithm
   *
   * The function filters RTT samples from the last RTT to find
   * the current smallest propagation delay + queueing delay (minRtt).
   * We take the minimum to avoid the effects of delayed ACKs.
   *
   * The function also min-filters all RTT measurements seen to find the
   * propagation delay (baseRtt).
   *
   * \param tcb internal congestion state
   * \param segmentsAcked count of segments ACKed
   * \param rtt last RTT
   *
   */
  virtual void PktsAcked (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked,
                          const Time& rtt);

  /**
   * \brief Enable/disable IcTcp algorithm depending on the congestion state
   *
   * We only start a Vegas cycle when we are in normal congestion state (CA_OPEN state).
   *
   * \param tcb internal congestion state
   * \param newState new congestion state to which the TCP is going to switch
   */
  virtual void CongestionStateSet (Ptr<TcpSocketState> tcb,
                                   const TcpSocketState::TcpCongState_t newState);

  /**
   * \brief Adjust cwnd following IcTcp linear increase/decrease algorithm
   *
   * \param tcb internal congestion state
   * \param segmentsAcked count of segments ACKed
   */
  virtual void IncreaseWindow (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked);

  
  virtual uint32_t GetSsThresh (Ptr<const TcpSocketState> tcb,
                                uint32_t bytesInFlight);

  virtual Ptr<TcpCongestionOps> Fork ();

protected:
private:
  /**
   * \brief Enable IcTcp algorithm to start taking IcTcp samples
   *
   * 
   *
   * \param tcb internal congestion state
   */
  void EnableIcTcp (Ptr<TcpSocketState> tcb);

  /**
   * \brief Stop taking IcTcp samples
   */
  void DisableIcTcp ();

private:
       
};

} // namespace ns3

#endif // ICTCP_H
