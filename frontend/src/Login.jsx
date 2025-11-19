import React from 'react'
import API from './api'

export default function Login({onLogin}){
  const [email, setEmail] = React.useState('')
  const [password, setPassword] = React.useState('')
  const [err, setErr] = React.useState('')
  const [busy, setBusy] = React.useState(false)

  const submit = async (e) => {
    e.preventDefault()
    setBusy(true); setErr('')
    try {
      const data = await API.login(email, password)
      onLogin(data)
    } catch (e) {
      setErr(e.message)
    } finally {
      setBusy(false)
    }
  }

  const formStyle = {
    width: '100%',
    maxWidth: '400px'
  }

  const headerStyle = {
    display: 'flex', 
    alignItems: 'center', 
    gap: '12px', 
    marginBottom: '32px', 
    justifyContent: 'center'
  }

  const titleStyle = {
    fontSize: '20px', 
    fontWeight: '700', 
    color: '#ffffff',
    textShadow: '0 1px 2px rgba(0, 0, 0, 0.1)'
  }

  const inputGroupStyle = {
    display: 'grid', 
    gap: '16px'
  }

  const labelStyle = {
    fontSize: '14px',
    fontWeight: '500',
    color: 'rgba(255, 255, 255, 0.9)',
    marginBottom: '4px',
    textShadow: '0 1px 2px rgba(0, 0, 0, 0.1)'
  }

  const inputStyle = {
    width: '100%',
    padding: '12px 16px',
    fontSize: '16px',
    border: '1px solid rgba(255, 255, 255, 0.3)',
    borderRadius: '8px',
    backgroundColor: 'rgba(255, 255, 255, 0.15)',
    backdropFilter: 'blur(20px)',
    outline: 'none',
    transition: 'all 0.3s ease',
    color: '#000000ff',
    boxShadow: '0 8px 32px rgba(0, 0, 0, 0.1)'
  }

  const buttonStyle = {
    width: '100%',
    padding: '12px 24px',
    fontSize: '16px',
    fontWeight: '600',
    color: '#ffffff',
    backgroundColor: 'rgba(59, 130, 246, 0.8)',
    border: '1px solid rgba(255, 255, 255, 0.3)',
    borderRadius: '8px',
    cursor: 'pointer',
    backdropFilter: 'blur(20px)',
    transition: 'all 0.3s ease',
    marginTop: '8px',
    boxShadow: '0 8px 32px rgba(59, 130, 246, 0.2)'
  }

  const errorStyle = {
    color: '#dc2626',
    fontSize: '14px',
    padding: '8px 12px',
    backgroundColor: 'rgba(254, 242, 242, 0.8)',
    backdropFilter: 'blur(10px)',
    border: '1px solid rgba(254, 202, 202, 0.5)',
    borderRadius: '6px',
    marginTop: '8px'
  }

  return (
    <>
      <style>{`
        input::placeholder {
          color: rgba(255, 255, 255, 0.7) !important;
        }
      `}</style>
      <form onSubmit={submit} style={formStyle}>
      <div style={headerStyle}>
        <img src="/new-logo.png" alt="RateMyLandlord" style={{width: '32px', height: '32px'}} />
        <span style={titleStyle}>RateMyLandlord</span>
      </div>
      <div style={inputGroupStyle}>
        <div>
          <label style={labelStyle}>Email</label>
          <input 
            value={email} 
            onChange={e=>setEmail(e.target.value)} 
            placeholder="Enter your email address"
            style={inputStyle}
          />
        </div>
        <div>
          <label style={labelStyle}>Password</label>
          <input 
            type="password" 
            value={password} 
            onChange={e=>setPassword(e.target.value)} 
            placeholder="Enter your password"
            style={inputStyle}
          />
        </div>
        <button disabled={busy} style={buttonStyle}>{busy ? 'Signing in…' : 'Sign in'}</button>
        {err && <div style={errorStyle}>{err}</div>}
      </div>
      </form>
    </>
  )
}
