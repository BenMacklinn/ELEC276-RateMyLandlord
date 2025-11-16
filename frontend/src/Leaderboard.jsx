import React from 'react'
import { Link, useNavigate } from 'react-router-dom'
import API from './api'

function Navbar({user, onLoginClick, onSignupClick, onLogout}){
  const navStyle = {
    display: 'flex',
    alignItems: 'center',
    justifyContent: 'space-between',
    padding: '16px 32px',
    backgroundColor: 'rgba(255, 255, 255, 0.15)',
    backdropFilter: 'blur(20px)',
    border: '1px solid rgba(255, 255, 255, 0.3)',
    borderRadius: '20px',
    boxShadow: '0 8px 32px rgba(0, 0, 0, 0.1)',
    position: 'fixed',
    top: '20px',
    left: '50%',
    transform: 'translateX(-50%)',
    width: 'calc(100% - 40px)',
    maxWidth: '1200px',
    zIndex: 1000
  }

  const logoStyle = {
    display: 'flex',
    alignItems: 'center',
    gap: '8px',
    fontSize: '20px',
    fontWeight: '700',
    color: '#ffffff',
    textDecoration: 'none',
    textShadow: '0 1px 2px rgba(0, 0, 0, 0.1)'
  }

  const navLinksStyle = {
    display: 'flex',
    alignItems: 'center',
    gap: '24px'
  }

  const navLinkStyle = {
    display: 'flex',
    alignItems: 'center',
    gap: '6px',
    padding: '8px 16px',
    borderRadius: '20px',
    textDecoration: 'none',
    color: 'rgba(255, 255, 255, 0.9)',
    fontSize: '14px',
    fontWeight: '500',
    transition: 'all 0.2s ease',
    cursor: 'pointer',
    textShadow: '0 1px 2px rgba(0, 0, 0, 0.1)'
  }

  const activeNavLinkStyle = {
    ...navLinkStyle,
    backgroundColor: 'rgba(59, 130, 246, 0.8)',
    color: '#ffffff',
    backdropFilter: 'blur(10px)'
  }

  return (
    <div style={navStyle}>
      <Link to="/" style={logoStyle}>
        <img src="/new-logo.png" alt="RateMyLandlord" style={{width: '24px', height: '24px'}} />
        RateMyLandlord
      </Link>
      <div style={navLinksStyle}>
        <Link to="/" style={navLinkStyle}>
          <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
            <path d="M3 9l9-7 9 7v11a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2z"/>
            <polyline points="9,22 9,12 15,12 15,22"/>
          </svg>
          Home
        </Link>
        <Link to="/landlords" style={navLinkStyle}>
          <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
            <circle cx="11" cy="11" r="8"/>
            <path d="M21 21l-4.35-4.35"/>
          </svg>
          Search
        </Link>
        <div style={activeNavLinkStyle}>
          <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
            <path d="M12 2l3.09 6.26L22 9.27l-5 4.87 1.18 6.88L12 17.77l-6.18 3.25L7 14.14 2 9.27l6.91-1.01L12 2z"/>
          </svg>
          Leaderboard
        </div>
        {user?.admin ? (
          <Link to="/admin" style={navLinkStyle}>
            <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
              <path d="M12 2l8 3v7c0 6-8 10-8 10s-8-4-8-10V5z"/>
              <circle cx="12" cy="11" r="3"/>
            </svg>
            Admin
          </Link>
        ) : null}
        {!user ? (
          <>
            <button onClick={onLoginClick} style={{
              ...navLinkStyle,
              backgroundColor: 'rgba(255, 255, 255, 0.1)',
              border: '1px solid rgba(255, 255, 255, 0.3)',
              color: 'rgba(255, 255, 255, 0.9)',
              backdropFilter: 'blur(10px)'
            }}>Log in</button>
            <button onClick={onSignupClick} style={{
              ...navLinkStyle,
              backgroundColor: 'rgba(59, 130, 246, 0.8)',
              color: '#ffffff',
              border: 'none',
              backdropFilter: 'blur(10px)'
            }}>Sign up</button>
          </>
        ) : (
          <div style={{display:'flex', alignItems:'center', gap:8}}>
            <span style={{color: 'rgba(255, 255, 255, 0.9)', fontSize: '14px', textShadow: '0 1px 2px rgba(0, 0, 0, 0.1)'}}>Hi, {user.name}</span>
            <button onClick={onLogout} style={{
              ...navLinkStyle,
              backgroundColor: 'rgba(255, 255, 255, 0.1)',
              border: '1px solid rgba(255, 255, 255, 0.3)',
              color: 'rgba(255, 255, 255, 0.9)',
              backdropFilter: 'blur(10px)'
            }}>Log out</button>
          </div>
        )}
      </div>
    </div>
  )
}

function LeaderboardCard({landlord, rank, onClick}) {
  const cardStyle = {
    backgroundColor: 'rgba(255, 255, 255, 0.15)',
    backdropFilter: 'blur(20px)',
    border: '1px solid rgba(255, 255, 255, 0.3)',
    borderRadius: '20px',
    padding: '24px',
    boxShadow: '0 8px 32px rgba(0, 0, 0, 0.1)',
    transition: 'all 0.3s ease',
    cursor: 'pointer',
    display: 'flex',
    alignItems: 'center',
    gap: '20px'
  }

  const cardHoverStyle = {
    transform: 'translateY(-4px)',
    boxShadow: '0 12px 40px rgba(0, 0, 0, 0.15)',
    backgroundColor: 'rgba(255, 255, 255, 0.2)'
  }

  const [isHovered, setIsHovered] = React.useState(false)

  const rankStyle = {
    fontSize: '32px',
    fontWeight: '800',
    color: 'rgba(255, 255, 255, 0.9)',
    minWidth: '60px',
    textAlign: 'center',
    textShadow: '0 2px 4px rgba(0, 0, 0, 0.2)'
  }

  const medalStyle = {
    fontSize: '48px',
    textAlign: 'center',
    minWidth: '60px'
  }

  const contentStyle = {
    flex: '1'
  }

  const nameStyle = {
    fontSize: '24px',
    fontWeight: '700',
    color: '#ffffff',
    marginBottom: '8px',
    textShadow: '0 2px 4px rgba(0, 0, 0, 0.2)'
  }

  const infoStyle = {
    fontSize: '14px',
    color: 'rgba(255, 255, 255, 0.8)',
    marginBottom: '4px',
    display: 'flex',
    alignItems: 'center',
    gap: '8px',
    textShadow: '0 1px 2px rgba(0, 0, 0, 0.1)'
  }

  const ratingStyle = {
    fontSize: '32px',
    fontWeight: '800',
    color: '#fbbf24',
    display: 'flex',
    alignItems: 'center',
    gap: '12px',
    textShadow: '0 2px 4px rgba(0, 0, 0, 0.2)'
  }

  const getMedal = (rank) => {
    if (rank === 1) return '🥇'
    if (rank === 2) return '🥈'
    if (rank === 3) return '🥉'
    return `#${rank}`
  }

  const renderStars = (rating) => {
    const stars = []
    const fullStars = Math.floor(rating)
    const hasHalfStar = rating % 1 >= 0.5
    
    for (let i = 0; i < fullStars; i++) {
      stars.push('⭐')
    }
    if (hasHalfStar && fullStars < 5) {
      stars.push('✨')
    }
    
    return stars.join('')
  }

  return (
    <div 
      style={{...cardStyle, ...(isHovered ? cardHoverStyle : {})}}
      onClick={onClick}
      onMouseEnter={() => setIsHovered(true)}
      onMouseLeave={() => setIsHovered(false)}
    >
      <div style={rank <= 3 ? medalStyle : rankStyle}>
        {rank <= 3 ? getMedal(rank) : `#${rank}`}
      </div>
      
      <div style={contentStyle}>
        <div style={nameStyle}>{landlord.name}</div>
        <div style={infoStyle}>
          <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
            <path d="M22 16.92v3a2 2 0 0 1-2.18 2 19.79 19.79 0 0 1-8.63-3.07 19.5 19.5 0 0 1-6-6 19.79 19.79 0 0 1-3.07-8.67A2 2 0 0 1 4.11 2h3a2 2 0 0 1 2 1.72 12.84 12.84 0 0 0 .7 2.81 2 2 0 0 1-.45 2.11L8.09 9.91a16 16 0 0 0 6 6l1.27-1.27a2 2 0 0 1 2.11-.45 12.84 12.84 0 0 0 2.81.7A2 2 0 0 1 22 16.92z"/>
          </svg>
          {landlord.contact?.phone || 'No phone'}
        </div>
        <div style={infoStyle}>
          <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
            <path d="M3 9l9-7 9 7v11a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2z"/>
            <polyline points="9,22 9,12 15,12 15,22"/>
          </svg>
          {(landlord.properties||[]).length} {landlord.properties?.length === 1 ? 'property' : 'properties'}
        </div>
      </div>

      <div style={ratingStyle}>
        {renderStars(landlord.average_rating)}
        <span style={{fontSize: '20px'}}>{landlord.average_rating.toFixed(1)}</span>
        <span style={{fontSize: '14px', color: 'rgba(255, 255, 255, 0.8)'}}>
          ({landlord.review_count} {landlord.review_count === 1 ? 'review' : 'reviews'})
        </span>
      </div>
    </div>
  )
}

function LandlordModal({landlord, onClose}) {
  if (!landlord) return null

  const modalOverlayStyle = {
    position: 'fixed',
    top: 0,
    left: 0,
    right: 0,
    bottom: 0,
    backgroundColor: 'rgba(0, 0, 0, 0.4)',
    backdropFilter: 'blur(8px)',
    zIndex: 2000,
    display: 'flex',
    alignItems: 'center',
    justifyContent: 'center',
    padding: '20px'
  }

  const modalContentStyle = {
    backgroundColor: 'rgba(255, 255, 255, 0.15)',
    backdropFilter: 'blur(20px)',
    border: '1px solid rgba(255, 255, 255, 0.3)',
    borderRadius: '20px',
    padding: '40px',
    maxWidth: '800px',
    width: '100%',
    maxHeight: '80vh',
    overflowY: 'auto',
    boxShadow: '0 8px 32px rgba(0, 0, 0, 0.1)',
    position: 'relative'
  }

  const closeButtonStyle = {
    position: 'absolute',
    top: '20px',
    right: '20px',
    backgroundColor: 'rgba(255, 255, 255, 0.1)',
    backdropFilter: 'blur(10px)',
    border: '1px solid rgba(255, 255, 255, 0.3)',
    fontSize: '24px',
    cursor: 'pointer',
    color: 'rgba(255, 255, 255, 0.9)',
    padding: '8px',
    borderRadius: '50%',
    width: '40px',
    height: '40px',
    display: 'flex',
    alignItems: 'center',
    justifyContent: 'center',
    transition: 'all 0.2s ease'
  }

  const modalTitleStyle = {
    fontSize: '28px',
    fontWeight: '700',
    color: '#ffffff',
    marginBottom: '20px',
    textAlign: 'center',
    textShadow: '0 1px 2px rgba(0, 0, 0, 0.1)'
  }

  const modalDetailStyle = {
    fontSize: '14px',
    color: 'rgba(255, 255, 255, 0.9)',
    marginBottom: '8px',
    padding: '8px 12px',
    backgroundColor: 'rgba(59, 130, 246, 0.2)',
    backdropFilter: 'blur(10px)',
    border: '1px solid rgba(255, 255, 255, 0.2)',
    borderRadius: '6px',
    textShadow: '0 1px 2px rgba(0, 0, 0, 0.1)'
  }

  const propertyStyle = {
    backgroundColor: 'rgba(255, 255, 255, 0.1)',
    backdropFilter: 'blur(10px)',
    border: '1px solid rgba(255, 255, 255, 0.2)',
    borderRadius: '12px',
    padding: '20px',
    marginBottom: '16px'
  }

  const propertyTitleStyle = {
    fontSize: '18px',
    fontWeight: '600',
    color: '#ffffff',
    marginBottom: '8px',
    textShadow: '0 1px 2px rgba(0, 0, 0, 0.1)'
  }

  const unitStyle = {
    fontSize: '14px',
    color: 'rgba(255, 255, 255, 0.8)',
    marginBottom: '4px',
    textShadow: '0 1px 2px rgba(0, 0, 0, 0.1)'
  }

  return (
    <div style={modalOverlayStyle} onClick={onClose}>
      <div style={modalContentStyle} onClick={e => e.stopPropagation()}>
        <button style={closeButtonStyle} onClick={onClose}>×</button>
        
        <div style={modalTitleStyle}>{landlord.name}</div>
        
        <div style={modalDetailStyle}>
          <strong>Email:</strong> {landlord.contact?.email}
        </div>
        <div style={modalDetailStyle}>
          <strong>Phone:</strong> {landlord.contact?.phone}
        </div>
        <div style={modalDetailStyle}>
          <strong>Properties:</strong> {(landlord.properties||[]).length}
        </div>
        <div style={modalDetailStyle}>
          <strong>Average Rating:</strong> {landlord.average_rating.toFixed(1)} ⭐ ({landlord.review_count} reviews)
        </div>
        
        <div style={{marginTop: '30px'}}>
          <h3 style={{fontSize: '20px', fontWeight: '600', color: '#ffffff', marginBottom: '16px', textShadow: '0 1px 2px rgba(0, 0, 0, 0.1)'}}>
            Property Listings
          </h3>
          
          {(landlord.properties||[]).map((property, idx) => (
            <div key={idx} style={propertyStyle}>
              <div style={propertyTitleStyle}>
                {property.address?.street}, {property.address?.city}, {property.address?.state}
              </div>
              
              {property.unit_details?.map((unit, unitIdx) => (
                <div key={unitIdx} style={unitStyle}>
                  <strong>Unit {unit.unit_number}:</strong> {unit.bedrooms} bedroom, {unit.bathrooms} bathroom - ${unit.rent}/month
                </div>
              ))}
            </div>
          ))}
        </div>
      </div>
    </div>
  )
}

export default function Leaderboard({user, onLoginClick, onSignupClick, onLogout}) {
  const navigate = useNavigate()
  const [leaderboard, setLeaderboard] = React.useState([])
  const [loading, setLoading] = React.useState(true)
  const [error, setError] = React.useState('')
  const [selectedLandlord, setSelectedLandlord] = React.useState(null)

  React.useEffect(() => {
    API.getLeaderboard()
      .then(data => {
        setLeaderboard(data)
      })
      .catch(err => {
        setError(err.message)
      })
      .finally(() => {
        setLoading(false)
      })
  }, [])

  const pageStyle = {
    minHeight: '100vh',
    background: 'linear-gradient(135deg, rgba(96, 165, 250, 0.8) 0%, rgba(59, 130, 246, 0.8) 100%), url("/hero-image.jpg")',
    backgroundSize: 'cover',
    backgroundPosition: 'center',
    backgroundRepeat: 'no-repeat',
    width: '100%',
    overflowX: 'hidden'
  }

  const contentStyle = {
    maxWidth: '1200px',
    margin: '0 auto',
    padding: '120px 24px 40px'
  }

  const headerStyle = {
    textAlign: 'center',
    marginBottom: '48px'
  }

  const titleStyle = {
    fontSize: '3rem',
    fontWeight: '800',
    color: '#ffffff',
    textAlign: 'center',
    marginBottom: '12px',
    lineHeight: '1.1',
    textShadow: '0 2px 4px rgba(0, 0, 0, 0.2)'
  }

  const subtitleStyle = {
    fontSize: '1.1rem',
    color: '#ffffff',
    textAlign: 'center',
    opacity: '0.95',
    textShadow: '0 1px 2px rgba(0, 0, 0, 0.1)'
  }

  const listStyle = {
    display: 'flex',
    flexDirection: 'column',
    gap: '20px'
  }

  const errorStyle = {
    color: '#dc2626',
    backgroundColor: 'rgba(254, 242, 242, 0.95)',
    backdropFilter: 'blur(20px)',
    padding: '16px 20px',
    borderRadius: '12px',
    marginBottom: '24px',
    border: '1px solid rgba(254, 202, 202, 0.5)',
    boxShadow: '0 4px 12px rgba(220, 38, 38, 0.1)'
  }

  const loadingStyle = {
    textAlign: 'center',
    color: 'rgba(255, 255, 255, 0.9)',
    fontSize: '18px',
    padding: '48px 0',
    textShadow: '0 1px 2px rgba(0, 0, 0, 0.1)'
  }

  const emptyStyle = {
    textAlign: 'center',
    color: 'rgba(255, 255, 255, 0.9)',
    fontSize: '18px',
    padding: '48px 0',
    backgroundColor: 'rgba(255, 255, 255, 0.15)',
    backdropFilter: 'blur(20px)',
    borderRadius: '20px',
    border: '1px solid rgba(255, 255, 255, 0.3)',
    textShadow: '0 1px 2px rgba(0, 0, 0, 0.1)'
  }

  if (loading) {
    return (
      <div style={pageStyle}>
        <Navbar user={user} onLoginClick={onLoginClick} onSignupClick={onSignupClick} onLogout={onLogout} />
        <div style={contentStyle}>
          <div style={loadingStyle}>Loading leaderboard...</div>
        </div>
      </div>
    )
  }

  if (error) {
    return (
      <div style={pageStyle}>
        <Navbar user={user} onLoginClick={onLoginClick} onSignupClick={onSignupClick} onLogout={onLogout} />
        <div style={contentStyle}>
          <div style={errorStyle}>{error}</div>
        </div>
      </div>
    )
  }

  return (
    <div style={pageStyle}>
      <Navbar user={user} onLoginClick={onLoginClick} onSignupClick={onSignupClick} onLogout={onLogout} />
      
      <div style={contentStyle}>
        <div style={headerStyle}>
          <h1 style={titleStyle}>🏆 Landlord Leaderboard</h1>
          <p style={subtitleStyle}>Top-rated landlords based on user reviews</p>
        </div>

        {leaderboard.length === 0 ? (
          <div style={emptyStyle}>
            No landlords found. Be the first to write a review!
          </div>
        ) : (
          <div style={listStyle}>
            {leaderboard.map((landlord, index) => (
              <LeaderboardCard
                key={landlord.landlord_id}
                landlord={landlord}
                rank={index + 1}
                onClick={() => setSelectedLandlord(landlord)}
              />
            ))}
          </div>
        )}
      </div>

      {selectedLandlord && (
        <LandlordModal landlord={selectedLandlord} onClose={() => setSelectedLandlord(null)} />
      )}
    </div>
  )
}
