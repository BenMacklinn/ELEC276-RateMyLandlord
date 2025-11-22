import React from 'react'
import { useLocation, useNavigate } from 'react-router-dom'
import API from './api'

export default function ReportReview({ user }) {
  const location = useLocation()
  const navigate = useNavigate()
  const review = location.state?.review || {}

  const [title, setTitle] = React.useState(review.title || '')
  const [text, setText] = React.useState(review.review || '')
  const [reason, setReason] = React.useState('')
  // no longer collect an optional email; use logged-in user email if present
  const [busy, setBusy] = React.useState(false)
  const [err, setErr] = React.useState('')

  const submit = async (e) => {
    e.preventDefault()
    setErr('')
    setBusy(true)
    try {
      await API.reportReview(review.id || review.review_id || '', title, text, reason, user?.email || '')
      navigate(-1)
    } catch (e) {
      setErr(e.message)
    } finally {
      setBusy(false)
    }
  }

  const pageStyle = { minHeight: '100vh', background: 'linear-gradient(135deg, rgba(96, 165, 250, 0.8) 0%, rgba(59, 130, 246, 0.8) 100%)', padding: '120px 24px 40px' }
  const panelStyle = { maxWidth: 800, margin: '0 auto', backgroundColor: 'rgba(255,255,255,0.06)', borderRadius: 12, padding: 20, border: '1px solid rgba(255,255,255,0.08)', color: '#fff' }

  return (
    <div style={pageStyle}>
      <div style={panelStyle}>
        <h2 style={{marginTop:0}}>Report Review</h2>
        <p style={{opacity:0.9}}>Report this review to admins for moderation.</p>
        <form onSubmit={submit}>
          <div style={{marginBottom:12}}>
            <label style={{display:'block', marginBottom:6}}>Review Title</label>
            <input value={title} onChange={e=>setTitle(e.target.value)} style={{width:'100%', padding:8, borderRadius:6}} />
          </div>
          <div style={{marginBottom:12}}>
            <label style={{display:'block', marginBottom:6}}>Review Text</label>
            <textarea value={text} onChange={e=>setText(e.target.value)} rows={6} style={{width:'100%', padding:8, borderRadius:6}} />
          </div>
          <div style={{marginBottom:12}}>
            <label style={{display:'block', marginBottom:6}}>Reason for reporting</label>
            <textarea value={reason} onChange={e=>setReason(e.target.value)} rows={4} style={{width:'100%', padding:8, borderRadius:6}} />
          </div>
          {/* Optional email removed per UX request; current signed-in user email will be used if available */}
          {err && <div style={{color:'#fecaca', marginBottom:12}}>{err}</div>}
          <div style={{display:'flex', gap:12}}>
            <button disabled={busy} style={{padding:'8px 14px', borderRadius:8, background:'#10b981', color:'#fff', border:'none', cursor:'pointer'}}>{busy ? 'Sending…' : 'Send Report'}</button>
            <button type='button' onClick={()=>navigate(-1)} style={{padding:'8px 14px', borderRadius:8, background:'#94a3b8', color:'#fff', border:'none', cursor:'pointer'}}>Cancel</button>
          </div>
        </form>
      </div>
    </div>
  )
}
