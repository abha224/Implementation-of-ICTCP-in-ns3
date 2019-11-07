#include "ictcp.h"
#include "ns3/log.h"
#include "rtt-estimator.h" //getting vars : m_estimatedRtt, T
#include "tcp-option-rfc793.h"// mss value
#include "bandwidth-manager.h" //actual bandwidth

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("IcTcp");
NS_OBJECT_ENSURE_REGISTERED (IcTcp);


/*include somewhere:
*/

TypeId
  IcTcp::GetTypeId (void)
  {
    static TypeId tid = TypeId ("ns3::IcTcp")
    .SetParent<TcpNewReno> ()
    .AddConstructor<IcTcp> ()
    .AddTraceSource("EstimatedBW", "The estimated bandwidth",
                    MakeTraceSourceAccessor(&IcTcp::m_currentBW),
                    "ns3::TracedValueCallback::Double")
    .AddAttribute ("ThroughputRatio",
                   "Threshold value for updating beta",
                   DoubleValue (0.2),
                   MakeDoubleAccessor (&TcpHtcp::m_throughputRatio),
                   MakeDoubleChecker<double> ())
    .SetGroupName ("Internet")
    ;
    return tid;
  }

IcTcp::IcTcp (void)
  : TcpNewReno (),
    m_alpha (0.9),// bwa calc
    m_beta (0.85),// for throughput
    m_gamma1 (0.1),//gamma1 =0.1
    m_gamma2 (0.5), //gamma2= 0.5
    m_baseRtt (Time::Max ()),
    m_minRtt (Time::Max ()),
    m_doingIcTcpNow (true),
    m_begSndNxt (0)
    m_currentBW (0),
    //m_lastSampleBW (0),
    m_lastBW (0),
    m_dataSent (0),
    m_lastCon (0),
    m_throughput (0),
   

{
  NS_LOG_FUNCTION (this);
}

IcTcp::IcTcp (const IcTcp& sock)
  : TcpNewReno (sock),
    m_alpha (sock.m_alpha),
    m_beta (sock.m_beta),
    m_gamma (sock.m_gamma),
    m_baseRtt (sock.m_baseRtt),
    m_minRtt (sock.m_minRtt),
    m_doingIcTcpNow (true),
    m_begSndNxt (0),
    m_throughputRatio (sock.m_throughputRatio),
    m_throughput (sock.m_throughput),
    m_currentBW (sock.m_currentBW),
    m_lastSampleBW (sock.m_lastSampleBW),
    m_lastBW (sock.m_lastBW),
    m_lastCon (sock.m_lastCon),
    m_dataSent (sock.m_dataSent)
{
  NS_LOG_FUNCTION (this);
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
  
   if (tcb->m_congState == TcpSocketState::CA_OPEN)
    {
      m_dataSent += segmentsAcked * tcb->m_segmentSize;
    }  
    m_throughput = static_cast<uint32_t> (m_dataSent / (Simulator::Now ().GetSeconds () - m_lastCon.GetSeconds ()));    
if (rtt.IsZero ())
    {
      return;
    }
    
    m_minRtt = std::min (m_minRtt, rtt);
    NS_LOG_DEBUG ("Updated m_minRtt = " << m_minRtt);
    
    m_baseRtt = std::min (m_baseRtt, rtt);
    NS_LOG_DEBUG ("Updated m_baseRtt = " << m_baseRtt);
    
  
        //need estimated rtt

}


// need to change!!!!
void
  IcTcp::EnableIcTcp (Ptr<TcpSocketState> tcb)
  {
    NS_LOG_FUNCTION (this << tcb);// check for left shift
    m_doingIcTcpNow = true;
    m_begSndNxt = tcb->m_nextTxSequence;// right edge during the last rtt
    m_cntRtt = 0;//specific to vegas declaration, no of rtt measurements seen during connections
    m_minRtt = Time::Max ();//rtt mearsurements during last rtt
  }

void
  IcTcp::DisableIcTcp ()
  {
    NS_LOG_FUNCTION (this);
    
    m_doingIcTcpNow = false;
  }

//estimate the bandwidth, taken from westwood
void
IcTcp::EstimateBW (const Time &rtt, Ptr<TcpSocketState> tcb)
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT (!rtt.IsZero ());

  m_currentBW = m_ackedSegments * tcb->m_segmentSize / rtt.GetSeconds ();

  if (m_pType == TcpWestwood::WESTWOODPLUS)
    {
      m_IsCount = false;
    }

  m_ackedSegments = 0;
  NS_LOG_LOGIC ("Estimated BW: " << m_currentBW);

  // Filter the BW sample

  double alpha = 0.9;

  if (m_fType == TcpWestwood::NONE)
    {
    }
  else if (m_fType == TcpWestwood::TUSTIN)
    {
      double sample_bwe = m_currentBW;
      m_currentBW = (alpha * m_lastBW) + ((1 - alpha) * ((sample_bwe + m_lastSampleBW) / 2));
      m_lastSampleBW = sample_bwe;
      m_lastBW = m_currentBW;
    }

  NS_LOG_LOGIC ("Estimated BW after filtering: " << m_currentBW);
}
//defined in ops.h
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
//in 

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

    NS_LOG_LOGIC ("CurrentBW: " << m_currentBW << " minRtt: " << tcb->m_minRtt << " ssthresh: " <<  m_currentBW * static_cast<double> (tcb->m_minRtt.GetSeconds ()));
        m_throughput = 0;
        m_dataSent = 0;
  }

} // namespace ns3
