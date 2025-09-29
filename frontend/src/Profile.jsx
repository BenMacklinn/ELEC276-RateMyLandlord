import React from 'react'
import API from './api'

export default function Profile({token, onLogout}){
  const [me, setMe] = React.useState(null)
  const [err, setErr] = React.useState('')

  React.useEffect(()=>{
    let ok = true
    API.me(token).then(d => { if(ok) setMe(d) }).catch(e=> setErr(e.message))
    return ()=>{ ok=false }
  }, [token])

  return (
    <div>
      <div style={{display: 'flex', alignItems: 'center', gap: '8px', marginBottom: '24px', justifyContent: 'center'}}>
        <img src="/new-logo.png" alt="RateMyLandlord" style={{width: '24px', height: '24px'}} />
        <span style={{fontSize: '18px', fontWeight: '600', color: '#1f2937'}}>RateMyLandlord</span>
      </div>
      <h2>Profile</h2>
      {err && <div style={{color:'crimson'}}>{err}</div>}
      {me ? (
        <div>
          <div><b>Name:</b> {me.name}</div>
          <div><b>Email:</b> {me.email}</div>
        </div>
      ) : <div>Loading…</div>}
      <button style={{marginTop:12}} onClick={onLogout}>Log out</button>
    </div>
  )
}
