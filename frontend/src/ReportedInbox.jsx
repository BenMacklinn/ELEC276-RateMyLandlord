import React from 'react'
import { useLocation, useNavigate } from 'react-router-dom'

export default function ReportedInbox({ user }) {
  const location = useLocation()
  const navigate = useNavigate()
  const [loading, setLoading] = React.useState(true)
  const [err, setErr] = React.useState('')
  const [reports, setReports] = React.useState([])
  const [selected, setSelected] = React.useState(null)

  React.useEffect(() => {
    // Ensure this page was reached via the Admin button (frontend-level guard)
    if (!location.state || !location.state.fromAdmin) {
      navigate('/admin', { replace: true })
      return
    }

    const token = localStorage.getItem('token')

    // Attempt to fetch reported reviews from backend. If backend not implemented yet,
    // fall back to an empty list so the page still works.
    // use a helper so we can refresh after actions
    const fetchReports = async () => {
      setLoading(true)
      setErr('')
      try {
        const res = await fetch('/api/admin/reported', {
          headers: token ? { 'Authorization': `Bearer ${token}` } : {}
        })
        if (!res.ok) {
          const errorData = await res.json().catch(() => ({}))
          if (res.status === 401) {
            setErr('Unauthorized: Please log in as admin.')
            setReports([])
          } else {
            // Backend endpoint exists but returned error
            setErr(`Backend error (${res.status}): ${errorData.error || 'Unknown error'}`)
            setReports([])
          }
        } else {
          const j = await res.json().catch(() => ({}))
          setReports(j.reports || [])
          if (j.reports && j.reports.length === 0) {
            setErr('') // Clear error if empty array is valid
          }
        }
      } catch (e) {
        // network error; fallback to frontend public sample if present
        try {
          const fallback = await fetch('/reported.json')
          const j2 = await fallback.json().catch(() => ({}))
          setReports(j2.reports || [])
          setErr('Loaded local sample reports (network to backend failed).')
        } catch (e2) {
          setErr('Failed to reach server; showing local view.')
          setReports([])
        }
      } finally {
        setLoading(false)
      }
    }

    fetchReports()
  }, [])

  const [notice, setNotice] = React.useState('')

  const handleAction = async (id, action) => {
    const token = localStorage.getItem('token')
    if (!token) {
      setNotice('Missing admin token; sign in as admin first.')
      setTimeout(() => setNotice(''), 2500)
      return
    }

    setNotice('Processing...')
    try {
      const url = `/api/admin/reported/${id}/${action === 'approve' ? 'approve' : 'deny'}`
      const res = await fetch(url, {
        method: 'POST',
        headers: {
          'Authorization': `Bearer ${token}`
        }
      })
      if (!res.ok) {
        const j = await res.json().catch(() => ({}))
        setNotice(j.error || `Server error: ${res.status}`)
        setTimeout(() => setNotice(''), 3000)
        return
      }

      const j = await res.json().catch(() => ({}))
      setNotice(j.message || 'Action complete')
      // refresh reports list
      setTimeout(() => setNotice(''), 1500)
      // re-fetch reports from backend (or fallback)
      setLoading(true)
      try {
        const r2 = await fetch('/api/admin/reported', {
          headers: token ? { 'Authorization': `Bearer ${token}` } : {}
        })
        if (r2.ok) {
          const j2 = await r2.json().catch(() => ({}))
          setReports(j2.reports || [])
        } else {
          const fallback = await fetch('/reported.json')
          const j3 = await fallback.json().catch(() => ({}))
          setReports(j3.reports || [])
        }
      } catch (e) {
        try {
          const fallback = await fetch('/reported.json')
          const j3 = await fallback.json().catch(() => ({}))
          setReports(j3.reports || [])
        } catch (e2) {
          setReports([])
        }
      } finally {
        setLoading(false)
      }
    } catch (e) {
      setNotice('Network error')
      setTimeout(() => setNotice(''), 2500)
    }
  }

  const pageStyle = {
    minHeight: '100vh',
    background: 'linear-gradient(135deg, rgba(96, 165, 250, 0.8) 0%, rgba(59, 130, 246, 0.8) 100%)',
    padding: '120px 24px 40px'
  }

  const containerStyle = {
    maxWidth: '1100px',
    margin: '0 auto',
    display: 'grid',
    gridTemplateColumns: '320px 1fr',
    gap: '20px'
  }

  const inboxStyle = {
    backgroundColor: 'rgba(255,255,255,0.06)',
    borderRadius: 12,
    padding: 12,
    border: '1.5px solid #2563eb',
    height: '70vh',
    overflowY: 'auto',
    boxShadow: '0 2px 16px rgba(37,99,235,0.08)'
  }

  const itemStyle = (isSelected) => ({
    background: isSelected ? 'rgba(37,99,235,0.13)' : 'rgba(255,255,255,0.08)',
    padding: '16px 14px',
    borderRadius: 10,
    marginBottom: 14,
    cursor: 'pointer',
    border: isSelected ? '2.5px solid #2563eb' : '1.5px solid #dbeafe',
    boxShadow: isSelected ? '0 4px 16px rgba(37,99,235,0.13)' : '0 2px 8px rgba(37,99,235,0.07)',
    transition: 'all 0.18s cubic-bezier(.4,0,.2,1)'
  })

  const detailStyle = {
    background: '#f3f4f650', // light solid grey
    borderRadius: 16,
    padding: '32px 28px',
    border: '1.5px solid #cbd5e1',
    height: '70vh',
    overflowY: 'auto',
    color: '#1e293b',
    boxShadow: '0 4px 32px rgba(100,116,139,0.10)',
    display: 'flex',
    flexDirection: 'column',
    gap: '18px'
  }

  return (
    <div style={pageStyle}>
      <div style={containerStyle}>
        <div style={{gridColumn: '1 / 2'}}>
          <h3 style={{color:'#fff', marginTop:0}}>Reported Reviews</h3>
          <div style={inboxStyle}>
            {loading ? (
              <div style={{color:'#e5e7eb'}}>Loading…</div>
            ) : err ? (
              <div style={{color:'#fecaca'}}>{err}</div>
            ) : reports.length === 0 ? (
              <div style={{color:'#e5e7eb', padding:16}}>No reported reviews yet. When reports arrive they will appear here.</div>
            ) : (
              reports.map((r) => (
                <div key={r.id} style={itemStyle(selected && selected.id === r.id)} onClick={() => setSelected(r)}>
                  <div style={{fontWeight:700, color:'#fff'}}>{r.title || 'Untitled review'}</div>
                  <div style={{fontSize:12, opacity:0.85, color:'#e5e7eb'}}>By: {r.reported_by || 'Unknown'}</div>
                  <div style={{fontSize:12, opacity:0.75, color:'#c7d2fe', marginTop:6}}>Review id: {r.review_id}</div>
                </div>
              ))
            )}
          </div>
        </div>

        <div style={{gridColumn: '2 / 3'}}>
          <h3 style={{color:'#fff', marginTop:0}}>Details</h3>
          <div style={detailStyle}>
            {notice && <div style={{background:'rgba(255,255,255,0.06)', padding:8, borderRadius:6, marginBottom:12, color:'#fff'}}>{notice}</div>}
            {!selected ? (
              <div style={{color:'#64748b', fontWeight:500, fontSize:18, textAlign:'center', marginTop:40}}>Select a report from the left to see details and take action.</div>
            ) : (
              <div style={{display:'flex', flexDirection:'column', gap:24}}>
                <div style={{background:'#f1f5f9', color:'#1e293b', borderRadius:12, padding:'18px 20px', boxShadow:'0 2px 12px rgba(100,116,139,0.10)', border:'1.5px solid #cbd5e1'}}>
                  <h2 style={{margin:'0 0 8px 0', fontWeight:800, fontSize:22, letterSpacing:'-0.5px', color:'#334155'}}>{selected.title || 'Untitled review'}</h2>
                  <div style={{fontSize:15, opacity:0.95, marginTop:2}}>Reported by: <span style={{fontWeight:600}}>{selected.reported_by || 'Unknown'}</span></div>
                  <div style={{fontSize:13, opacity:0.8, marginTop:4}}>Review id: <span style={{fontWeight:500}}>{selected.review_id}</span></div>
                  <div style={{fontSize:14, marginTop:10, color:'#be185d', fontWeight:700, background:'#fdf2f8', borderRadius:6, padding:'8px 12px', display:'inline-block', marginTop:14}}>Reason for Report: {selected.reason || 'No reason provided'}</div>
                </div>
                <div style={{background:'#e2e8f0', borderRadius:10, padding:'18px 20px', boxShadow:'0 2px 8px rgba(100,116,139,0.07)', border:'1.5px solid #cbd5e1', color:'#1e293b', fontSize:16, fontWeight:500, whiteSpace:'pre-wrap', marginTop:4}}>
                  <span style={{fontWeight:700, color:'#334155'}}>Content of Review:</span><br/>{selected.review || 'No review text available.'}
                </div>
                <div style={{display:'flex', gap:16, marginTop:8}}>
                  <button onClick={() => handleAction(selected.id, 'approve')} style={{padding:'10px 22px', borderRadius:8, background:'#10b981', color:'#fff', border:'none', cursor:'pointer', fontWeight:700, fontSize:16, boxShadow:'0 2px 8px rgba(16,185,129,0.10)'}}>Approve</button>
                  <button onClick={() => handleAction(selected.id, 'deny')} style={{padding:'10px 22px', borderRadius:8, background:'#ef4444', color:'#fff', border:'none', cursor:'pointer', fontWeight:700, fontSize:16, boxShadow:'0 2px 8px rgba(239,68,68,0.10)'}}>Deny</button>
                </div>
              </div>
            )}
          </div>
        </div>
      </div>
    </div>
  )
}
