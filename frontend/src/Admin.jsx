import React from 'react'
import API from './api'
import { Link } from 'react-router-dom'

const defaultTools = [
  {
    title: 'Landlord Request Inbox',
    description: 'Review the queue of new landlord submissions that need an approval or follow-up.',
    cta: 'Open landlord requests'
  },
  {
    title: 'Reported Reviews Inbox',
    description: 'See reviews that have been reported by tenants or landlords and decide what action is needed.',
    cta: 'Open reported reviews'
  }
]

export default function Admin({ user }) {
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

  const gridStyle = {
    display: 'grid',
    gridTemplateColumns: 'repeat(auto-fit, minmax(260px, 1fr))',
    gap: '20px'
  }

  const cardStyle = {
    backgroundColor: 'rgba(255, 255, 255, 0.12)',
    borderRadius: '18px',
    padding: '20px',
    border: '1px solid rgba(255, 255, 255, 0.2)',
    backdropFilter: 'blur(16px)',
    display: 'flex',
    flexDirection: 'column',
    gap: '12px',
    minHeight: '200px'
  }

  const cardTitleStyle = {
    fontSize: '1.25rem',
    fontWeight: '700'
  }

  const cardDescriptionStyle = {
    fontSize: '0.95rem',
    opacity: 0.85,
    flexGrow: 1,
    lineHeight: 1.4
  }

  const buttonStyle = {
    alignSelf: 'flex-start',
    padding: '10px 18px',
    borderRadius: '999px',
    border: 'none',
    cursor: 'pointer',
    backgroundColor: 'rgba(255, 255, 255, 0.9)',
    color: '#1d4ed8',
    fontWeight: 600,
    boxShadow: '0 4px 12px rgba(255, 255, 255, 0.25)'
  }

  return (
    <div style={pageStyle}>
      <div style={panelStyle}>
        <header style={headerStyle}>
          <h1 style={titleStyle}>Admin Console</h1>
          <p style={subtitleStyle}>
            You are signed in as <strong>{user?.name || 'Admin'}</strong> ({user?.email}). From here you
            control the quality of landlord listings and keep the marketplace trustworthy.
          </p>
        </header>
        <div style={gridStyle}>
          {defaultTools.map((tool) => (
            <div key={tool.title} style={cardStyle}>
              <div style={cardTitleStyle}>{tool.title}</div>
              <div style={cardDescriptionStyle}>{tool.description}</div>

              {tool.title === 'Landlord Request Inbox' ? (
                <Link to="/admin/requests">
                  <button style={buttonStyle} type="button">
                    {tool.cta}
                  </button>
                </Link>
              ) : (
                <button style={buttonStyle} type="button">
                  {tool.cta}
                </button>
              )}
            </div>
          ))}
        </div>
      </div>
    </div>
  )
}
