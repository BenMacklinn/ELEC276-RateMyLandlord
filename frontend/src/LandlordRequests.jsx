import React from 'react'
import API from './api'

export default function LandlordRequests({ user }) {
  const [requests, setRequests] = React.useState([])
  const [loading, setLoading] = React.useState(true)
  const [error, setError] = React.useState('')
  const [actionId, setActionId] = React.useState(null)

  const pageStyle = {
    minHeight: '100vh',
    background: 'linear-gradient(135deg, rgba(96, 165, 250, 0.8) 0%, rgba(59, 130, 246, 0.8) 100%)',
    padding: '120px 24px 40px'
  }

  const panelStyle = {
    maxWidth: '1000px',
    margin: '0 auto',
    backgroundColor: 'rgba(255, 255, 255, 0.15)',
    borderRadius: '24px',
    padding: '32px',
    border: '1px solid rgba(255, 255, 255, 0.3)',
    backdropFilter: 'blur(20px)',
    boxShadow: '0 8px 32px rgba(0, 0, 0, 0.1)',
    color: '#ffffff'
  }

  const headerStyle = {
    marginBottom: '24px'
  }

  const titleStyle = {
    fontSize: '2.5rem',
    fontWeight: '800',
    marginBottom: '12px'
  }

  const subtitleStyle = {
    fontSize: '1rem',
    opacity: 0.9,
    maxWidth: '640px',
    lineHeight: 1.5
  }

  const infoTextStyle = {
    fontSize: '0.95rem',
    opacity: 0.9,
    marginTop: '8px'
  }

  const listStyle = {
    display: 'flex',
    flexDirection: 'column',
    gap: '16px',
    marginTop: '24px'
  }

  const cardStyle = {
    backgroundColor: 'rgba(255, 255, 255, 0.12)',
    borderRadius: '18px',
    padding: '16px 20px',
    border: '1px solid rgba(255, 255, 255, 0.2)',
    backdropFilter: 'blur(16px)',
    display: 'flex',
    flexDirection: 'column',
    gap: '8px'
  }

  const cardHeaderStyle = {
    display: 'flex',
    justifyContent: 'space-between',
    alignItems: 'center',
    gap: '12px'
  }

  const landlordNameStyle = {
    fontSize: '1.1rem',
    fontWeight: '700'
  }

  const metaRowStyle = {
    display: 'flex',
    flexWrap: 'wrap',
    gap: '12px',
    fontSize: '0.9rem',
    opacity: 0.9
  }

  const labelStyle = {
    fontWeight: 600
  }

  const detailsStyle = {
    fontSize: '0.9rem',
    opacity: 0.9,
    marginTop: '4px'
  }

  const statusPillStyle = (status) => ({
    padding: '4px 10px',
    borderRadius: '999px',
    fontSize: '0.8rem',
    fontWeight: 600,
    textTransform: 'uppercase',
    letterSpacing: '0.03em',
    backgroundColor:
      status === 'approved'
        ? 'rgba(22, 163, 74, 0.2)'
        : status === 'rejected'
        ? 'rgba(220, 38, 38, 0.2)'
        : 'rgba(234, 179, 8, 0.2)',
    border:
      status === 'approved'
        ? '1px solid rgba(74, 222, 128, 0.8)'
        : status === 'rejected'
        ? '1px solid rgba(248, 113, 113, 0.8)'
        : '1px solid rgba(252, 211, 77, 0.8)'
  })

  const buttonRowStyle = {
    display: 'flex',
    gap: '8px',
    marginTop: '8px'
  }

  const buttonBaseStyle = {
    padding: '8px 14px',
    borderRadius: '999px',
    border: 'none',
    cursor: 'pointer',
    fontSize: '0.9rem',
    fontWeight: 600
  }

  const approveButtonStyle = {
    ...buttonBaseStyle,
    backgroundColor: 'rgba(34, 197, 94, 0.9)',
    color: '#022c22',
    boxShadow: '0 4px 12px rgba(22, 163, 74, 0.4)'
  }

  const rejectButtonStyle = {
    ...buttonBaseStyle,
    backgroundColor: 'rgba(248, 113, 113, 0.9)',
    color: '#450a0a',
    boxShadow: '0 4px 12px rgba(220, 38, 38, 0.4)'
  }

  const disabledButtonStyle = {
    opacity: 0.6,
    cursor: 'default'
  }

  const errorStyle = {
    marginTop: '8px',
    padding: '8px 10px',
    borderRadius: '8px',
    backgroundColor: 'rgba(220, 38, 38, 0.2)',
    border: '1px solid rgba(248, 113, 113, 0.8)',
    fontSize: '0.9rem'
  }

  const emptyStyle = {
    marginTop: '16px',
    fontSize: '0.95rem',
    opacity: 0.9
  }

  // Guard: only admins can see this page
  if (!user?.admin) {
    return (
      <div style={pageStyle}>
        <div style={panelStyle}>
          <h1 style={titleStyle}>Landlord Request Inbox</h1>
          <p style={subtitleStyle}>You must be an admin to view this page.</p>
        </div>
      </div>
    )
  }

  const loadRequests = React.useCallback(async () => {
    setLoading(true)
    setError('')
    try {
      const data = await API.getLandlordRequests()
      setRequests(data)
    } catch (e) {
      setError(e.message || 'Failed to load landlord requests')
    } finally {
      setLoading(false)
    }
  }, [])

  React.useEffect(() => {
    loadRequests()
  }, [loadRequests])

  const handleApprove = async (id) => {
    setActionId(id)
    setError('')
    try {
      await API.approveLandlordRequest(id)
      // remove from list on success
      setRequests((prev) => prev.filter((r) => r.id !== id))
    } catch (e) {
      setError(e.message || 'Failed to approve request')
    } finally {
      setActionId(null)
    }
  }

  const handleReject = async (id) => {
    const reason = window.prompt(
      'Optional: provide a reason for rejecting this request (or leave blank):',
      ''
    )
    if (reason === null) return // user cancelled

    setActionId(id)
    setError('')
    try {
      await API.rejectLandlordRequest(id, reason.trim())
      setRequests((prev) => prev.filter((r) => r.id !== id))
    } catch (e) {
      setError(e.message || 'Failed to reject request')
    } finally {
      setActionId(null)
    }
  }

  const pendingRequests = requests.filter(
    (r) => !r.status || r.status === 'pending'
  )

  return (
    <div style={pageStyle}>
      <div style={panelStyle}>
        <header style={headerStyle}>
          <h1 style={titleStyle}>Landlord Request Inbox</h1>
          <p style={subtitleStyle}>
            Review landlord requests submitted by users. Approving a request
            creates a new landlord in <code>landlords.json</code>; rejecting it
            removes it from the pending queue.
          </p>
          <p style={infoTextStyle}>
            Only requests with status <strong>pending</strong> appear here.
          </p>
        </header>

        {error && <div style={errorStyle}>{error}</div>}

        {loading ? (
          <div style={emptyStyle}>Loading landlord requests…</div>
        ) : pendingRequests.length === 0 ? (
          <div style={emptyStyle}>No pending landlord requests.</div>
        ) : (
          <div style={listStyle}>
            {pendingRequests.map((req) => {
              const createdAt = req.created_at
                ? new Date(req.created_at).toLocaleString()
                : 'Unknown'

              return (
                <div key={req.id} style={cardStyle}>
                  <div style={cardHeaderStyle}>
                    <div>
                      <div style={landlordNameStyle}>
                        #{req.id} – {req.landlord_name || 'Unnamed landlord'}
                      </div>
                      <div style={metaRowStyle}>
                        <span>
                          <span style={labelStyle}>Requested by:</span>{' '}
                          {req.user_name || 'Anonymous'} ({req.user_email})
                        </span>
                         <span>
                            <span style={labelStyle}>Landlord contact:</span>{' '}
                            {req.landlord_name || 'Unknown'} · {req.landlord_email || 'no email'} ·{' '}
                            {req.landlord_phone || 'no phone'}
                        </span>
                        {req.property_address && (
                          <span>
                            <span style={labelStyle}>Property:</span>{' '}
                            {req.property_address}
                          </span>
                        )}
                        <span>
                          <span style={labelStyle}>Created:</span> {createdAt}
                        </span>
                      </div>
                    </div>
                    <div style={statusPillStyle(req.status || 'pending')}>
                      {req.status || 'pending'}
                    </div>
                  </div>

                  {req.details && (
                    <div style={detailsStyle}>
                      <span style={labelStyle}>Details: </span>
                      {req.details}
                    </div>
                  )}

                  <div style={buttonRowStyle}>
                    <button
                      type="button"
                      style={
                        actionId === req.id
                          ? { ...approveButtonStyle, ...disabledButtonStyle }
                          : approveButtonStyle
                      }
                      disabled={actionId === req.id}
                      onClick={() => handleApprove(req.id)}
                    >
                      {actionId === req.id
                        ? 'Working…'
                        : 'Approve & add landlord'}
                    </button>
                    <button
                      type="button"
                      style={
                        actionId === req.id
                          ? { ...rejectButtonStyle, ...disabledButtonStyle }
                          : rejectButtonStyle
                      }
                      disabled={actionId === req.id}
                      onClick={() => handleReject(req.id)}
                    >
                      {actionId === req.id ? 'Working…' : 'Reject'}
                    </button>
                  </div>
                </div>
              )
            })}
          </div>
        )}
      </div>
    </div>
  )
}
