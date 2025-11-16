import React from 'react'
import { Link } from 'react-router-dom'
import API from './api'

function ReviewList({ landlordId, onRatingCalculated }) {
  const [reviews, setReviews] = React.useState([])
  const [loading, setLoading] = React.useState(true)

  React.useEffect(() => {
    let ok = true
    API.getLandlordReviews(landlordId)
      .then(reviews => { 
        if (ok) { 
          setReviews(reviews)
          setLoading(false)
          // compute average and notify parent
          if (onRatingCalculated) {
            const count = reviews.length
            const avg = count ? (reviews.reduce((s, r) => s + (r.rating||0), 0) / count) : 0
            onRatingCalculated(parseFloat(avg.toFixed(1)), count)
          }
        }
      })
      .catch((error) => { 
        console.error('Error fetching reviews:', error)
        if (ok) setLoading(false) 
      })
    return () => { ok = false }
  }, [landlordId, onRatingCalculated])

  if (loading) return <div style={{color: 'rgba(255, 255, 255, 0.8)'}}>Loading reviews...</div>
  if (!reviews.length) return <div style={{color: 'rgba(255, 255, 255, 0.8)'}}>No reviews yet</div>

  return (
    <div style={{ marginTop: '12px' }}>
      {reviews.map(review => (
        <div key={review.id} style={{ 
          backgroundColor: 'rgba(255, 255, 255, 0.1)',
          backdropFilter: 'blur(10px)',
          border: '1px solid rgba(255, 255, 255, 0.2)',
          borderRadius: '8px',
          padding: '16px',
          marginBottom: '12px'
        }}>
          <div style={{ 
            display: 'flex', 
            justifyContent: 'space-between',
            color: '#ffffff'
          }}>
            <div style={{ fontWeight: 'bold' }}>{review.title}</div>
            <div>Rating: {review.rating}/5</div>
          </div>
          <div style={{ 
            marginTop: '8px',
            color: 'rgba(255, 255, 255, 0.9)'
          }}>{review.review}</div>
          <div style={{ 
            fontSize: '0.8em', 
            color: 'rgba(255, 255, 255, 0.7)', 
            marginTop: '8px'
          }}>
            {new Date(review.created_at).toLocaleDateString()}
          </div>
        </div>
      ))}
    </div>
  )
}

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
        <div style={activeNavLinkStyle}>
          <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
            <circle cx="11" cy="11" r="8"/>
            <path d="M21 21l-4.35-4.35"/>
          </svg>
          Search
        </div>
        <Link to="/leaderboard" style={navLinkStyle}>
          <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
            <path d="M12 2l3.09 6.26L22 9.27l-5 4.87 1.18 6.88L12 17.77l-6.18 3.25L7 14.14 2 9.27l6.91-1.01L12 2z"/>
          </svg>
          Leaderboard
        </Link>
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
              backgroundColor: 'rgba(59, 130, 246, 0.8)',
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
              backgroundColor: 'rgba(59, 130, 246, 0.8)',
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

function SearchResults({err, loading, list}){
  const [expandedCard, setExpandedCard] = React.useState(null)
  const [landlordAvg, setLandlordAvg] = React.useState(null)
  const [landlordReviewCount, setLandlordReviewCount] = React.useState(0)

  const resultsStyle = {
    maxWidth: '1200px',
    margin: '0 auto',
    padding: '40px 24px',
    fontFamily: 'system-ui, sans-serif'
  }

  const gridStyle = {
    display: 'grid',
    gridTemplateColumns: 'repeat(3, 1fr)',
    gap: '20px',
    listStyle: 'none',
    padding: '0',
    margin: '0'
  }

  const errorStyle = {
    color: '#dc2626',
    backgroundColor: '#fef2f2',
    padding: '12px 16px',
    borderRadius: '8px',
    marginBottom: '24px',
    border: '1px solid #fecaca'
  }

  const loadingStyle = {
    textAlign: 'center',
    color: '#6b7280',
    fontSize: '18px',
    padding: '48px 0'
  }

  const itemStyle = {
    backgroundColor: 'rgba(255, 255, 255, 0.15)',
    backdropFilter: 'blur(20px)',
    border: '1px solid rgba(255, 255, 255, 0.3)',
    borderRadius: '16px',
    padding: '16px',
    boxShadow: '0 8px 32px rgba(0, 0, 0, 0.1)',
    transition: 'all 0.3s ease',
    cursor: 'pointer',
    display: 'flex',
    flexDirection: 'column'
  }

  const nameStyle = {
    fontSize: '18px',
    fontWeight: '600',
    color: '#ffffff',
    marginBottom: '8px',
    textShadow: '0 1px 2px rgba(0, 0, 0, 0.1)'
  }

  const statsStyle = {
    fontSize: '14px',
    color: 'rgba(255, 255, 255, 0.9)',
    marginBottom: '4px',
    textShadow: '0 1px 2px rgba(0, 0, 0, 0.1)',
    display: 'flex',
    justifyContent: 'space-between',
    alignItems: 'center'
  }

  const propertyStyle = {
    fontSize: '11px',
    color: 'rgba(255, 255, 255, 0.8)',
    marginTop: '8px',
    padding: '8px',
    backgroundColor: 'rgba(255, 255, 255, 0.1)',
    backdropFilter: 'blur(10px)',
    border: '1px solid rgba(255, 255, 255, 0.2)',
    borderRadius: '6px'
  }

  const propertiesContainerStyle = {
    maxHeight: expandedCard ? 'none' : '0',
    overflow: 'hidden',
    transition: 'max-height 0.3s ease',
    marginTop: '12px'
  }

  const expandedContentStyle = {
    marginTop: '12px',
    overflow: 'hidden'
  }

  const detailStyle = {
    fontSize: '13px',
    color: 'rgba(255, 255, 255, 0.9)',
    marginBottom: '6px',
    textShadow: '0 1px 2px rgba(0, 0, 0, 0.1)'
  }

  if (err) {
    return (
      <div style={resultsStyle}>
        <div style={errorStyle}>{err}</div>
      </div>
    )
  }

  if (loading) {
    return (
      <div style={resultsStyle}>
        <div style={loadingStyle}>Searching…</div>
      </div>
    )
  }

  if (!list.length) {
    return (
      <div style={resultsStyle}>
        <div style={loadingStyle}>No landlords found.</div>
      </div>
    )
  }

  const handleCardClick = (landlordId) => {
    // Toggle: if this card is expanded, collapse it; otherwise expand it
    const newExpanded = expandedCard === landlordId ? null : landlordId
    setExpandedCard(newExpanded)
    if (newExpanded === null) {
      // reset rating display when closing
      setLandlordAvg(null)
      setLandlordReviewCount(0)
    }
  }

  // Modal overlay styles
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
    textShadow: '0 1px 2px rgba(0, 0, 0, 0.1)',
    width: '300px',
    margin: '0 auto 8px',
    display: 'flex',
    alignItems: 'center',
    justifyContent: 'center'
  }

  const modalPropertyStyle = {
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
    <>
      <div style={resultsStyle}>
        <ul style={gridStyle}>
          {list.map((ll) => (
            <li 
              key={ll.landlord_id} 
              style={itemStyle}
              onClick={() => handleCardClick(ll.landlord_id)}
            >
              {/* Compact view - always visible */}
              <div style={nameStyle}>{ll.name}</div>
              <div style={statsStyle}>
                <span>{(ll.properties||[]).length} properties</span>
                <span>{ll.contact?.phone || 'No phone'}</span>
              </div>
            </li>
          ))}
        </ul>
      </div>

      {/* Modal overlay for expanded view */}
      {expandedCard && (
        <div style={modalOverlayStyle} onClick={() => setExpandedCard(null)}>
          <div style={modalContentStyle} onClick={e => e.stopPropagation()}>
            <button style={closeButtonStyle} onClick={() => setExpandedCard(null)}>×</button>
            
            {(() => {
              const selectedLandlord = list.find(ll => ll.landlord_id === expandedCard)
              if (!selectedLandlord) return null
              
              return (
                <>
                  <div style={modalTitleStyle}>{selectedLandlord.name}</div>
                  
                  <div style={modalDetailStyle}>
                    <strong style={{display: 'flex', alignItems: 'center', gap: '8px'}}>
                      <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
                        <path d="M4 4h16c1.1 0 2 .9 2 2v12c0 1.1-.9 2-2 2H4c-1.1 0-2-.9-2-2V6c0-1.1.9-2 2-2z"/>
                        <polyline points="22,6 12,13 2,6"/>
                      </svg>
                      Email:
                    </strong> {selectedLandlord.contact?.email}
                  </div>
                  <div style={modalDetailStyle}>
                    <strong style={{display: 'flex', alignItems: 'center', gap: '8px'}}>
                      <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
                        <path d="M22 16.92v3a2 2 0 0 1-2.18 2 19.79 19.79 0 0 1-8.63-3.07 19.5 19.5 0 0 1-6-6 19.79 19.79 0 0 1-3.07-8.67A2 2 0 0 1 4.11 2h3a2 2 0 0 1 2 1.72 12.84 12.84 0 0 0 .7 2.81 2 2 0 0 1-.45 2.11L8.09 9.91a16 16 0 0 0 6 6l1.27-1.27a2 2 0 0 1 2.11-.45 12.84 12.84 0 0 0 2.81.7A2 2 0 0 1 22 16.92z"/>
                      </svg>
                      Phone:
                    </strong> {selectedLandlord.contact?.phone}
                  </div>
                  <div style={modalDetailStyle}>
                    <strong style={{display: 'flex', alignItems: 'center', gap: '8px'}}>
                      <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
                        <path d="M3 9l9-7 9 7v11a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2z"/>
                        <polyline points="9,22 9,12 15,12 15,22"/>
                      </svg>
                      Properties:
                    </strong> {(selectedLandlord.properties||[]).length}
                  </div>
                  
                  <div style={{marginTop: '30px'}}>
                    <h3 style={{fontSize: '20px', fontWeight: '600', color: '#ffffff', marginBottom: '16px', textShadow: '0 1px 2px rgba(0, 0, 0, 0.1)'}}>
                      Property Listings
                    </h3>
                    
                    {(selectedLandlord.properties||[]).map((property, idx) => (
                      <div key={idx} style={modalPropertyStyle}>
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

                    <div style={{marginTop: '20px'}}>
                      <div style={modalDetailStyle}>
                        <strong style={{display: 'flex', alignItems: 'center', gap: '8px'}}>
                          <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
                            <path d="M12 2l3.09 6.26L22 9.27l-5 4.87 1.18 6.88L12 17.77l-6.18 3.25L7 14.14 2 9.27l6.91-1.01L12 2z"/>
                          </svg>
                          Average Rating:
                        </strong>
                        {selectedLandlord.average_rating ? `${selectedLandlord.average_rating.toFixed(1)} ⭐ (${selectedLandlord.review_count} reviews)` : (landlordAvg ? `${landlordAvg} ⭐ (${landlordReviewCount} reviews)` : 'No ratings yet')}
                      </div>

                      <h3 style={{fontSize: '20px', fontWeight: '600', color: '#ffffff', marginTop: '16px', marginBottom: '16px', textShadow: '0 1px 2px rgba(0, 0, 0, 0.1)'}}>
                        Reviews
                      </h3>
                      <ReviewList landlordId={selectedLandlord.landlord_id} onRatingCalculated={(avg,count)=>{ setLandlordAvg(avg); setLandlordReviewCount(count); }} />
                    </div>
                  </div>
                </>
              )
            })()}
          </div>
        </div>
      )}
    </>
  )
}

export default function Landlords({user, onLoginClick, onSignupClick, onLogout}){
  const [q, setQ] = React.useState('')
  const [list, setList] = React.useState([])
  const [err, setErr] = React.useState('')
  const [loading, setLoading] = React.useState(false)

  const doSearch = async (e)=>{
    e && e.preventDefault()
    setErr(''); setLoading(true)
    try {
      const results = await API.searchLandlords(q.trim())
      setList(results)
    } catch(e){
      setErr(e.message)
    } finally {
      setLoading(false)
    }
  }

  React.useEffect(()=>{ doSearch() }, [])

  const pageStyle = {
    minHeight: '100vh',
    background: 'linear-gradient(135deg, rgba(96, 165, 250, 0.8) 0%, rgba(59, 130, 246, 0.8) 100%), url("/hero-image.jpg")',
    backgroundSize: 'cover',
    backgroundPosition: 'center',
    backgroundRepeat: 'no-repeat',
    width: '100%',
    overflowX: 'hidden'
  }

  const headerStyle = {
    padding: '120px 24px 60px',
    textAlign: 'center'
  }

  const titleStyle = {
    fontSize: '3rem',
    fontWeight: '800',
    color: '#ffffff',
    textAlign: 'center',
    marginBottom: '12px',
    lineHeight: '1.1'
  }

  const subtitleStyle = {
    fontSize: '1.1rem',
    color: '#ffffff',
    textAlign: 'center',
    marginBottom: '32px',
    maxWidth: '600px',
    lineHeight: '1.4',
    opacity: '0.95',
    margin: '0 auto 32px'
  }

  const searchContainerStyle = {
    display: 'flex',
    gap: '12px',
    maxWidth: '600px',
    width: '100%',
    margin: '0 auto'
  }

  const searchInputStyle = {
    flex: '1',
    padding: '16px 20px',
    fontSize: '16px',
    border: '1px solid rgba(255, 255, 255, 0.3)',
    borderRadius: '12px',
    backgroundColor: 'rgba(255, 255, 255, 0.15)',
    backdropFilter: 'blur(20px)',
    boxShadow: '0 8px 32px rgba(0, 0, 0, 0.1)',
    outline: 'none',
    color: '#ffffff',
    transition: 'all 0.3s ease'
  }

  const searchButtonStyle = {
    padding: '16px 20px',
    backgroundColor: 'rgba(255, 255, 255, 0.2)',
    color: '#ffffff',
    border: '1px solid rgba(255, 255, 255, 0.3)',
    borderRadius: '12px',
    fontSize: '16px',
    fontWeight: '600',
    cursor: 'pointer',
    display: 'flex',
    alignItems: 'center',
    gap: '8px',
    backdropFilter: 'blur(20px)',
    boxShadow: '0 8px 32px rgba(0, 0, 0, 0.1)',
    transition: 'all 0.3s ease'
  }

  return (
    <>
      <style>{`
        input::placeholder {
          color: rgba(255, 255, 255, 0.7) !important;
        }
      `}</style>
      <div style={pageStyle}>
        <Navbar 
          user={user}
          onLoginClick={onLoginClick}
          onSignupClick={onSignupClick}
          onLogout={onLogout}
        />
        
        <div style={headerStyle}>
          <h1 style={titleStyle}>Landlords</h1>
          <p style={subtitleStyle}>Search and browse all available landlords and their properties</p>
          
          <form onSubmit={doSearch} style={searchContainerStyle}>
          <input
            value={q}
            onChange={e=>setQ(e.target.value)}
            placeholder="Search landlords by name..."
            style={searchInputStyle}
          />
          <button 
            type="submit"
            style={searchButtonStyle}
          >
            <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
              <circle cx="11" cy="11" r="8"/>
              <path d="M21 21l-4.35-4.35"/>
            </svg>
            Search
          </button>
          </form>
        </div>
        
        <SearchResults err={err} loading={loading} list={list} />
      </div>
    </>
  )
}
