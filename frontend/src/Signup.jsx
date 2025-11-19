import React from 'react'
import API from './api'

export default function Signup({onSignup}){
  const [name, setName] = React.useState('')
  const [email, setEmail] = React.useState('')
  const [password, setPassword] = React.useState('')
  const [code, setCode] = React.useState('')
  const [err, setErr] = React.useState('')
  const [info, setInfo] = React.useState('')
  const [busy, setBusy] = React.useState(false)
  const [resending, setResending] = React.useState(false)
  const [stage, setStage] = React.useState('email')

  const submit = async (e) => {
    e.preventDefault()
    setErr('')
    setInfo('')

    if(stage === 'email') {
      setBusy(true)
      try {
        await API.requestVerification(email)
        setStage('code')
        setInfo(`We sent a 6 digit code to ${email}.`)
      } catch (ex) {
        setErr(ex.message)
      } finally {
        setBusy(false)
      }
      return
    }

    if(stage === 'code') {
      setBusy(true)
      try {
        await API.verifyEmailCode(email, code)
        setStage('details')
        setInfo('Email verified. Finish creating your account below.')
      } catch (ex) {
        setErr(ex.message)
      } finally {
        setBusy(false)
      }
      return
    }

    setBusy(true)
    try {
      const data = await API.signup(name, email, password)
      onSignup(data)
    } catch (ex) {
      setErr(ex.message)
    } finally {
      setBusy(false)
    }
  }

  const resend = async () => {
    setErr('')
    setInfo('')
    setResending(true)
    try {
      await API.requestVerification(email)
      setInfo(`A new code was sent to ${email}.`)
    } catch (ex) {
      setErr(ex.message)
    } finally {
      setResending(false)
    }
  }

  const resetEmail = () => {
    setStage('email')
    setCode('')
    setName('')
    setPassword('')
    setInfo('')
    setErr('')
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

  const infoStyle = {
    color: '#2563eb',
    fontSize: '14px',
    padding: '8px 12px',
    backgroundColor: 'rgba(219, 234, 254, 0.8)',
    backdropFilter: 'blur(10px)',
    border: '1px solid rgba(191, 219, 254, 0.6)',
    borderRadius: '6px',
    marginTop: '8px'
  }

  const buttonLabel = stage === 'email'
    ? 'Send verification code'
    : stage === 'code'
      ? 'Verify code'
      : 'Create account'

  const busyLabel = stage === 'email'
    ? 'Sending…'
    : stage === 'code'
      ? 'Verifying…'
      : 'Creating…'

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
        {stage === 'details' && (
          <div>
            <label style={labelStyle}>Name</label>
            <input 
              value={name} 
              onChange={e=>setName(e.target.value)} 
              placeholder="Enter your full name" 
              required
              style={inputStyle}
              disabled={busy || resending}
            />
          </div>
        )}
        <div>
          <label style={labelStyle}>Email</label>
          <input 
            type="email" 
            value={email} 
            onChange={e=>setEmail(e.target.value)} 
            placeholder="Enter your email address" 
            required={stage === 'email'}
            disabled={stage !== 'email' || busy}
            style={inputStyle}
          />
        </div>
        {stage === 'code' && (
          <div>
            <label style={labelStyle}>Verification code</label>
            <input 
              value={code}
              onChange={e=>setCode(e.target.value.replace(/\D/g, '').slice(0, 6))}
              placeholder="Enter the 6 digit code"
              inputMode="numeric"
              pattern="[0-9]{6}"
              required
              style={inputStyle}
              disabled={busy || resending}
            />
          </div>
        )}
        {stage === 'details' && (
          <div>
            <label style={labelStyle}>Password</label>
            <input 
              type="password" 
              value={password} 
              onChange={e=>setPassword(e.target.value)} 
              placeholder="Create a secure password" 
              required
              style={inputStyle}
              disabled={busy || resending}
            />
          </div>
        )}
        <button disabled={busy || resending || (stage === 'details' && (!name || !password))} style={buttonStyle}>
          {busy ? busyLabel : buttonLabel}
        </button>
        {stage === 'code' && (
          <>
            <button type="button" onClick={resend} disabled={busy || resending} style={{...buttonStyle, marginTop: '4px'}}>
              {resending ? 'Resending…' : 'Resend code'}
            </button>
            <button type="button" onClick={resetEmail} disabled={busy || resending} style={{...buttonStyle, marginTop: '4px'}}>
              Change email
            </button>
          </>
        )}
        {stage === 'details' && (
          <button type="button" onClick={resetEmail} disabled={busy || resending} style={{...buttonStyle, marginTop: '4px'}}>
            Use a different email
          </button>
        )}
        {info && <div style={infoStyle}>{info}</div>}
        {err && <div style={errorStyle}>{err}</div>}
      </div>
      </form>
    </>
  )
}
