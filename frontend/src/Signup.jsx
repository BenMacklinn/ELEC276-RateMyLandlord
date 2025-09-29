import React from 'react'
import API from './api'

export default function Signup({onSignup}){
  const [name, setName] = React.useState('')
  const [email, setEmail] = React.useState('')
  const [password, setPassword] = React.useState('')
  const [err, setErr] = React.useState('')
  const [busy, setBusy] = React.useState(false)

  const submit = async (e) => {
    e.preventDefault()
    setBusy(true); setErr('')
    try {
      const data = await API.signup(name, email, password)
      onSignup(data)
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
    color: '#1f2937',
    textShadow: '0 1px 2px rgba(0, 0, 0, 0.1)'
  }

  const inputGroupStyle = {
    display: 'grid', 
    gap: '16px'
  }

  const labelStyle = {
    fontSize: '14px',
    fontWeight: '500',
    color: '#374151',
    marginBottom: '4px'
  }

  const inputStyle = {
    width: '100%',
    padding: '12px 16px',
    fontSize: '16px',
    border: '1px solid rgba(229, 231, 235, 0.8)',
    borderRadius: '8px',
    backgroundColor: 'rgba(255, 255, 255, 0.9)',
    backdropFilter: 'blur(10px)',
    outline: 'none',
    transition: 'all 0.2s ease'
  }

  const buttonStyle = {
    width: '100%',
    padding: '12px 24px',
    fontSize: '16px',
    fontWeight: '600',
    color: '#ffffff',
    backgroundColor: 'rgba(59, 130, 246, 0.8)',
    border: '1px solid rgba(255, 255, 255, 0.2)',
    borderRadius: '8px',
    cursor: 'pointer',
    backdropFilter: 'blur(10px)',
    transition: 'all 0.2s ease',
    marginTop: '8px'
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
    <form onSubmit={submit} style={formStyle}>
      <div style={headerStyle}>
        <img src="/new-logo.png" alt="RateMyLandlord" style={{width: '32px', height: '32px'}} />
        <span style={titleStyle}>RateMyLandlord</span>
      </div>
      <div style={inputGroupStyle}>
        <div>
          <label style={labelStyle}>Name</label>
          <input 
            value={name} 
            onChange={e=>setName(e.target.value)} 
            placeholder="Enter your full name" 
            required
            style={inputStyle}
          />
        </div>
        <div>
          <label style={labelStyle}>Email</label>
          <input 
            type="email" 
            value={email} 
            onChange={e=>setEmail(e.target.value)} 
            placeholder="Enter your email address" 
            required
            style={inputStyle}
          />
        </div>
        <div>
          <label style={labelStyle}>Password</label>
          <input 
            type="password" 
            value={password} 
            onChange={e=>setPassword(e.target.value)} 
            placeholder="Create a secure password" 
            required
            style={inputStyle}
          />
        </div>
        <button disabled={busy} style={buttonStyle}>{busy ? 'Creating…' : 'Create account'}</button>
        {err && <div style={errorStyle}>{err}</div>}
      </div>
    </form>
  )
}
