#include "ictcp.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("IcTcp");
NS_OBJECT_ENSURE_REGISTERED (IcTcp);

TypeId
  IcTcp::GetTypeId (void)
  {
    static TypeId tid = TypeId ("ns3::IcTcp")
    .SetParent<TcpNewReno> ()
    .AddConstructor<IcTcp> ()
    .SetGroupName ("Internet")
    ;
    return tid;
  }

IcTcp::IcTcp (void)
  : TcpNewReno ()
{
  NS_LOG_FUNCTION (this);
}

IcTcp::IcTcp (const IcTcp& sock)
  : TcpNewReno (sock)
{
  NS_LOG_FUNCTION (this);
}

IcTcp::~IcTcp (void)
{
  NS_LOG_FUNCTION (this);
}

Ptr<TcpCongestionOps>
  IcTcp::Fork (void)
  {
    return CopyObject<IcTcp> (this);
  }

void
  IcTcp::PktsAcked (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked,
                       const Time& rtt)
  {
    NS_LOG_FUNCTION (this << tcb << segmentsAcked << rtt);
    
    if (rtt.IsZero ())
    {
      return;
    }
    
    m_minRtt = std::min (m_minRtt, rtt);
    NS_LOG_DEBUG ("Updated m_minRtt = " << m_minRtt);
    
    m_baseRtt = std::min (m_baseRtt, rtt);
    NS_LOG_DEBUG ("Updated m_baseRtt = " << m_baseRtt);
    
    // Update RTT counter
    m_cntRtt++;
    NS_LOG_DEBUG ("Updated m_cntRtt = " << m_cntRtt);
  }

void
  IcTcp::EnableIcTcp (Ptr<TcpSocketState> tcb)
  {
    NS_LOG_FUNCTION (this << tcb);
    
    m_doingIcTcpNow = true;
    m_begSndNxt = tcb->m_nextTxSequence;
    m_cntRtt = 0;
    m_minRtt = Time::Max ();
  }

void
  IcTcp::DisableIcTcp ()
  {
    NS_LOG_FUNCTION (this);
    
    m_doingIcTcpNow = false;
  }

void
  IcTcp::CongestionStateSet (Ptr<TcpSocketState> tcb,
                                const TcpSocketState::TcpCongState_t newState)
  {
    NS_LOG_FUNCTION (this << tcb << newState);
    if (newState == TcpSocketState::CA_OPEN)
    {
      EnableIcTcp (tcb);
    }
    else
    {
      DisableIcTcp ();
    }
  }


//change it for IcTcp
void
  IcTcp::IncreaseWindow (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
  {
    NS_LOG_FUNCTION (this << tcb << segmentsAcked);
    
    if (!m_doingIcTcpNow)
    {
      // If IcTcp is not on, we follow NewReno algorithm
      NS_LOG_LOGIC ("IcTcp is not turned on, we follow NewReno algorithm.");
      TcpNewReno::IncreaseWindow (tcb, segmentsAcked);
      return;
    }
    
    if (tcb->m_lastAckedSeq >= m_begSndNxt)
    { // A IcTcp cycle has finished, we do Vegas cwnd adjustment every RTT.
      
      NS_LOG_LOGIC ("A Vegas cycle has finished, we adjust cwnd once per RTT.");
      
      // Save the current right edge for next Vegas cycle
      m_begSndNxt = tcb->m_nextTxSequence;
      
      /*
      * We perform IcTcp calculations only if we got enough RTT samples to
      * insure that at least 1 of those samples wasn't from a delayed ACK.
      */
      if (m_cntRtt <= 2)
      {  // We do not have enough RTT samples, so we should behave like Reno
        NS_LOG_LOGIC ("We do not have enough RTT samples to do IcTcp, so we behave like NewReno.");
        TcpNewReno::IncreaseWindow (tcb, segmentsAcked);
      }
      else
      {
        NS_LOG_LOGIC ("We have enough RTT samples to perform IcTcp calculations");
        /*
        * We have enough RTT samples to perform IcTcp algorithm.
        * Now we need to determine if rwnd should be increased or decreased
        *
        */
        
      }
  }

std::string
  IcTcp::GetName () const
  {
    return "IcTcp";
  }

uint32_t
  IcTcp::GetSsThresh (Ptr<const TcpSocketState> tcb,
                         uint32_t bytesInFlight)
  {
    NS_LOG_FUNCTION (this << tcb << bytesInFlight);
    return std::max (std::min (tcb->m_ssThresh.Get (), tcb->m_cWnd.Get () - tcb->m_segmentSize), 2 * tcb->m_segmentSize);
  }

} // namespace ns3
