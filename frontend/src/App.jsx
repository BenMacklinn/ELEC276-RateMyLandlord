import React from 'react'
import { Routes, Route, Link, Navigate } from 'react-router-dom'
import Login from './Login'
import Signup from './Signup'
import Landlords from './Landlords'
import Review from './Review'
import Leaderboard from './Leaderboard'
import Admin from './Admin'
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

function StatsBar({stats}) {
  const statsStyle = {
    backgroundColor: 'rgba(255, 255, 255, 0.15)',
    padding: '32px 40px',
    display: 'flex',
    justifyContent: 'center',
    alignItems: 'center',
    gap: '80px',
    flexWrap: 'wrap',
    borderRadius: '24px',
    backdropFilter: 'blur(20px)',
    border: '1px solid rgba(255, 255, 255, 0.3)',
    boxShadow: '0 8px 32px rgba(0, 0, 0, 0.1)',
    maxWidth: '800px',
    margin: '0 auto'
  }

  const statItemStyle = {
    display: 'flex',
    flexDirection: 'column',
    alignItems: 'center',
    gap: '12px',
    textAlign: 'center'
  }

  const iconStyle = {
    width: '56px',
    height: '56px',
    backgroundColor: '#3b82f6',
    borderRadius: '50%',
    display: 'flex',
    alignItems: 'center',
    justifyContent: 'center',
    color: '#ffffff',
    boxShadow: '0 4px 12px rgba(59, 130, 246, 0.3)'
  }

  const numberStyle = {
    fontSize: '2.5rem',
    fontWeight: '800',
    color: '#ffffff',
    lineHeight: '1',
    textShadow: '0 2px 4px rgba(0, 0, 0, 0.1)'
  }

  const labelStyle = {
    fontSize: '1rem',
    color: 'rgba(255, 255, 255, 0.9)',
    fontWeight: '500',
    textShadow: '0 1px 2px rgba(0, 0, 0, 0.1)'
  }

  return (
    <div style={statsStyle}>
      <div style={statItemStyle}>
        <div style={iconStyle}>
          <svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
            <path d="M17 21v-2a4 4 0 0 0-4-4H5a4 4 0 0 0-4 4v2"/>
            <circle cx="9" cy="7" r="4"/>
            <path d="M23 21v-2a4 4 0 0 0-3-3.87"/>
            <path d="M16 3.13a4 4 0 0 1 0 7.75"/>
          </svg>
        </div>
        <div style={numberStyle}>{stats?.landlords || 0}</div>
        <div style={labelStyle}>Landlords Posted</div>
      </div>
      
      <div style={statItemStyle}>
        <div style={iconStyle}>
          <svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
            <path d="M3 9l9-7 9 7v11a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2z"/>
            <polyline points="9,22 9,12 15,12 15,22"/>
          </svg>
        </div>
        <div style={numberStyle}>{stats?.properties || 0}</div>
        <div style={labelStyle}>Properties Posted</div>
      </div>
      
      <div style={statItemStyle}>
        <div style={iconStyle}>
          <svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
            <rect x="3" y="3" width="18" height="18" rx="2" ry="2"/>
            <line x1="9" y1="9" x2="15" y2="9"/>
            <line x1="9" y1="15" x2="15" y2="15"/>
          </svg>
        </div>
        <div style={numberStyle}>{stats?.units || 0}</div>
        <div style={labelStyle}>Units Available</div>
      </div>
    </div>
  )
}

function HeroSection({stats}){
  const heroStyle = {
    background: 'linear-gradient(135deg, rgba(96, 165, 250, 0.8) 0%, rgba(59, 130, 246, 0.8) 100%), url("/hero-image.jpg")',
    backgroundSize: 'cover',
    backgroundPosition: 'center',
    backgroundRepeat: 'no-repeat',
    minHeight: '100vh',
    width: '100%',
    display: 'flex',
    flexDirection: 'column',
    alignItems: 'center',
    justifyContent: 'center',
    padding: '80px 24px 40px',
    position: 'relative',
    overflow: 'hidden'
  }

  const cityIllustrationStyle = {
    position: 'absolute',
    bottom: '0',
    left: '0',
    right: '0',
    height: '200px',
    background: 'url("data:image/svg+xml,%3Csvg xmlns=\'http://www.w3.org/2000/svg\' viewBox=\'0 0 1200 200\'%3E%3Cpath d=\'M0,200 L0,120 Q50,100 100,120 L200,100 Q250,80 300,100 L400,80 Q450,60 500,80 L600,60 Q650,40 700,60 L800,40 Q850,20 900,40 L1000,20 Q1050,0 1100,20 L1200,0 L1200,200 Z\' fill=\'%23ffffff\' opacity=\'0.1\'/%3E%3C/svg%3E")',
    backgroundSize: 'cover',
    backgroundPosition: 'bottom'
  }

  const badgeStyle = {
    backgroundColor: 'rgba(255, 255, 255, 0.2)',
    color: '#ffffff',
    padding: '8px 16px',
    borderRadius: '20px',
    fontSize: '14px',
    fontWeight: '500',
    marginBottom: '24px',
    backdropFilter: 'blur(10px)'
  }

  const titleStyle = {
    fontSize: '3rem',
    fontWeight: '800',
    color: '#ffffff',
    textAlign: 'center',
    marginBottom: '12px',
    lineHeight: '1.1'
  }

  const taglineStyle = {
    fontSize: '1.1rem',
    color: '#ffffff',
    textAlign: 'center',
    marginBottom: '32px',
    maxWidth: '600px',
    lineHeight: '1.4',
    opacity: '0.95'
  }

  const searchContainerStyle = {
    display: 'flex',
    gap: '12px',
    maxWidth: '600px',
    width: '100%',
    marginBottom: '32px'
  }

  const searchInputStyle = {
    flex: '1',
    padding: '16px 20px',
    fontSize: '16px',
    border: 'none',
    borderRadius: '12px',
    backgroundColor: '#ffffff',
    boxShadow: '0 4px 6px rgba(0, 0, 0, 0.1)',
    outline: 'none'
  }

  const filterButtonStyle = {
    padding: '16px 20px',
    backgroundColor: '#ffffff',
    color: '#3b82f6',
    border: 'none',
    borderRadius: '12px',
    fontSize: '16px',
    fontWeight: '600',
    cursor: 'pointer',
    display: 'flex',
    alignItems: 'center',
    gap: '8px',
    boxShadow: '0 4px 6px rgba(0, 0, 0, 0.1)',
    transition: 'all 0.2s ease'
  }

  const filterButtonHoverStyle = {
    ...filterButtonStyle,
    backgroundColor: '#f8fafc',
    transform: 'translateY(-1px)',
    boxShadow: '0 6px 8px rgba(0, 0, 0, 0.15)'
  }

  const ctaButtonsStyle = {
    display: 'flex',
    gap: '16px',
    marginBottom: '80px'
  }

  const ctaButtonStyle = {
    padding: '16px 32px',
    backgroundColor: '#ffffff',
    color: '#3b82f6',
    border: 'none',
    borderRadius: '12px',
    fontSize: '16px',
    fontWeight: '600',
    cursor: 'pointer',
    transition: 'all 0.2s ease',
    boxShadow: '0 4px 6px rgba(0, 0, 0, 0.1)'
  }

  const ctaButtonHoverStyle = {
    ...ctaButtonStyle,
    backgroundColor: '#f8fafc',
    transform: 'translateY(-2px)',
    boxShadow: '0 8px 12px rgba(0, 0, 0, 0.15)'
  }

  return (
    <div style={heroStyle}>
      <div style={cityIllustrationStyle}></div>
      
      <h1 style={titleStyle}>RateMyLandlord</h1>
      
      <p style={taglineStyle}>
        Search and find landlord information. Browse properties and contact details to make informed decisions.
      </p>
      
      
       <div style={{textAlign: 'center', marginTop: '16px', display: 'flex', gap: '16px', justifyContent: 'center'}}>
         <Link 
           to="/landlords"
           style={{
             ...ctaButtonStyle,
             backgroundColor: 'rgba(255, 255, 255, 0.2)',
             color: '#ffffff',
             border: '1px solid rgba(255, 255, 255, 0.3)',
             textDecoration: 'none',
             display: 'inline-block'
           }}
           onMouseEnter={(e) => {
             Object.assign(e.target.style, {
               ...ctaButtonHoverStyle,
               backgroundColor: 'rgba(255, 255, 255, 0.3)'
             })
           }}
           onMouseLeave={(e) => {
             Object.assign(e.target.style, {
               ...ctaButtonStyle,
               backgroundColor: 'rgba(255, 255, 255, 0.2)',
               color: '#ffffff',
               border: '1px solid rgba(255, 255, 255, 0.3)',
               textDecoration: 'none'
             })
           }}
         >
           View All Landlords
         </Link>
         <Link 
           to="/review"
           style={{
             ...ctaButtonStyle,
             backgroundColor: 'rgba(255, 255, 255, 0.2)',
             color: '#ffffff',
             border: '1px solid rgba(255, 255, 255, 0.3)',
             textDecoration: 'none',
             display: 'inline-block'
           }}
           onMouseEnter={(e) => {
             Object.assign(e.target.style, {
               ...ctaButtonHoverStyle,
               backgroundColor: 'rgba(255, 255, 255, 0.3)'
             })
           }}
           onMouseLeave={(e) => {
             Object.assign(e.target.style, {
               ...ctaButtonStyle,
               backgroundColor: 'rgba(255, 255, 255, 0.2)',
               color: '#ffffff',
               border: '1px solid rgba(255, 255, 255, 0.3)',
               textDecoration: 'none'
             })
           }}
         >
           Write a Review
         </Link>
       </div>
       
       {stats && (
         <div style={{marginTop: '40px', width: '100%'}}>
           <StatsBar stats={stats} />
         </div>
       )}
      
    </div>
  )
}


function SearchResults({err, loading, list}){
  const resultsStyle = {
    maxWidth: '800px',
    margin: '48px auto',
    padding: '0 24px',
    fontFamily: 'system-ui, sans-serif'
  }

  const errorStyle = {
    color: '#dc2626',
    backgroundColor: 'rgba(254, 242, 242, 0.9)',
    backdropFilter: 'blur(10px)',
    padding: '16px 20px',
    borderRadius: '12px',
    marginBottom: '24px',
    border: '1px solid rgba(254, 202, 202, 0.5)',
    boxShadow: '0 4px 12px rgba(220, 38, 38, 0.1)'
  }

  const loadingStyle = {
    textAlign: 'center',
    color: '#6b7280',
    fontSize: '18px',
    padding: '48px 0',
    backgroundColor: 'rgba(255, 255, 255, 0.7)',
    backdropFilter: 'blur(10px)',
    borderRadius: '12px',
    margin: '20px 0',
    border: '1px solid rgba(255, 255, 255, 0.3)'
  }

  const listStyle = {
    listStyle: 'none',
    padding: '0',
    margin: '0'
  }

  const itemStyle = {
    backgroundColor: 'rgba(255, 255, 255, 0.9)',
    backdropFilter: 'blur(20px)',
    border: '1px solid rgba(255, 255, 255, 0.3)',
    borderRadius: '16px',
    padding: '24px',
    marginBottom: '20px',
    boxShadow: '0 8px 32px rgba(0, 0, 0, 0.1)',
    transition: 'all 0.3s ease'
  }

  const nameStyle = {
    fontSize: '18px',
    fontWeight: '600',
    color: '#1f2937',
    marginBottom: '8px'
  }

  const detailStyle = {
    fontSize: '14px',
    color: '#6b7280',
    marginBottom: '4px'
  }

  const propertyStyle = {
    fontSize: '12px',
    color: '#6b7280',
    marginTop: '12px',
    padding: '12px',
    backgroundColor: 'rgba(249, 250, 251, 0.8)',
    backdropFilter: 'blur(10px)',
    border: '1px solid rgba(229, 231, 235, 0.5)',
    borderRadius: '8px'
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

  return (
    <div style={resultsStyle}>
      <ul style={listStyle}>
        {list.map((ll) => (
          <li key={ll.landlord_id} style={itemStyle}>
            <div style={nameStyle}>{ll.name}</div>
            <div style={detailStyle}>{ll.contact?.email} · {ll.contact?.phone}</div>
            <div style={detailStyle}>{(ll.properties||[]).length} properties</div>
            {(ll.properties||[]).map((property, idx) => (
              <div key={idx} style={propertyStyle}>
                <strong>{property.address?.street}, {property.address?.city}, {property.address?.state}</strong>
                <br />
                {property.unit_details?.map((unit, unitIdx) => (
                  <span key={unitIdx}>
                    {unit.unit_number}: {unit.bedrooms}br/{unit.bathrooms}ba - ${unit.rent}/month
                    {unitIdx < property.unit_details.length - 1 ? ', ' : ''}
                  </span>
                ))}
              </div>
            ))}
                </li>
              ))}
            </ul>
    </div>
  )
}

function HomePage(){
  const [stats, setStats] = React.useState(null)
  const [statsLoading, setStatsLoading] = React.useState(true)

  React.useEffect(() => {
    API.getStats()
      .then((data) => {
        console.log('Stats received:', data)
        setStats(data)
      })
      .catch((error) => {
        console.log('Stats API failed, using fallback:', error)
        // Fallback to hardcoded stats if API fails
        setStats({ landlords: 2, properties: 3, units: 6 })
      })
      .finally(() => setStatsLoading(false))
  }, [])

  return (
    <>
      <HeroSection stats={!statsLoading ? stats : null} />
    </>
  )
}

export default function App(){
  const [token, setToken] = React.useState(localStorage.getItem('token'))
  const [user, setUser] = React.useState(null)
  const [userLoading, setUserLoading] = React.useState(!!token)
  const [showLogin, setShowLogin] = React.useState(false)
  const [showSignup, setShowSignup] = React.useState(false)

  React.useEffect(()=>{
    if(!token){
      setUser(null)
      setUserLoading(false)
      return
    }
    setUserLoading(true)
    API.me(token)
      .then(setUser)
      .catch(()=> setUser(null))
      .finally(()=> setUserLoading(false))
  }, [token])

  const handleAuthSuccess = (data) => {
    if(!data || !data.token) return
    localStorage.setItem('token', data.token)
    setToken(data.token)
    setUser({
      name: data.name,
      email: data.email,
      admin: data.admin ? 1 : 0
    })
  }

  const handleLogout = () => {
    localStorage.removeItem('token')
    setToken(null)
    setUser(null)
    setUserLoading(false)
  }

  const appStyle = {
    width: '100%',
    minHeight: '100vh',
    overflowX: 'hidden'
  }

  const loadingScreenStyle = {
    minHeight: '100vh',
    display: 'flex',
    alignItems: 'center',
    justifyContent: 'center',
    color: '#ffffff',
    fontSize: '18px',
    background: 'linear-gradient(135deg, rgba(96, 165, 250, 0.8) 0%, rgba(59, 130, 246, 0.8) 100%)'
  }

  return (
    <div style={appStyle}>
      <Routes>
        <Route path="/" element={
          <>
      <Navbar
        user={user}
        onLoginClick={()=>{ setShowLogin(true); setShowSignup(false) }}
        onSignupClick={()=>{ setShowSignup(true); setShowLogin(false) }}
        onLogout={handleLogout}
      />
            <HomePage />
          </>
        } />
        <Route path="/landlords" element={
          <Landlords
            user={user}
            onLoginClick={()=>{ setShowLogin(true); setShowSignup(false) }}
            onSignupClick={()=>{ setShowSignup(true); setShowLogin(false) }}
            onLogout={handleLogout}
          />
        } />
        <Route path="/review" element={
          <>
            <Navbar
              user={user}
              onLoginClick={()=>{ setShowLogin(true); setShowSignup(false) }}
              onSignupClick={()=>{ setShowSignup(true); setShowLogin(false) }}
              onLogout={handleLogout}
            />
            <Review />
          </>
        } />
        <Route path="/leaderboard" element={
          <Leaderboard
            user={user}
            onLoginClick={()=>{ setShowLogin(true); setShowSignup(false) }}
            onSignupClick={()=>{ setShowSignup(true); setShowLogin(false) }}
            onLogout={handleLogout}
          />
        } />
        <Route path="/admin" element={
          userLoading ? (
            <div style={loadingScreenStyle}>Checking permissions…</div>
          ) : user?.admin ? (
            <>
              <Navbar
                user={user}
                onLoginClick={()=>{ setShowLogin(true); setShowSignup(false) }}
                onSignupClick={()=>{ setShowSignup(true); setShowLogin(false) }}
                onLogout={handleLogout}
              />
              <Admin user={user} />
            </>
          ) : (
            <Navigate to="/" replace />
          )
        } />
      </Routes>

      {showLogin && (
        <div style={{position:'fixed', inset:0, background:'rgba(0,0,0,0.4)', backdropFilter: 'blur(8px)', zIndex: 2000, display: 'flex', alignItems: 'center', justifyContent: 'center', padding: '20px'}} onClick={()=>setShowLogin(false)}>
          <div style={{background:'rgba(255, 255, 255, 0.15)', backdropFilter: 'blur(20px)', border: '1px solid rgba(255, 255, 255, 0.3)', padding:32, maxWidth:420, width: '100%', borderRadius: 20, boxShadow: '0 8px 32px rgba(0,0,0,0.1)'}} onClick={e=>e.stopPropagation()}>
            <Login onLogin={(data)=>{ handleAuthSuccess(data); setShowLogin(false) }} />
          </div>
        </div>
      )}

      {showSignup && (
        <div style={{position:'fixed', inset:0, background:'rgba(0,0,0,0.4)', backdropFilter: 'blur(8px)', zIndex: 2000, display: 'flex', alignItems: 'center', justifyContent: 'center', padding: '20px'}} onClick={()=>setShowSignup(false)}>
          <div style={{background:'rgba(255, 255, 255, 0.15)', backdropFilter: 'blur(20px)', border: '1px solid rgba(255, 255, 255, 0.3)', padding:32, maxWidth:420, width: '100%', borderRadius: 20, boxShadow: '0 8px 32px rgba(0,0,0,0.1)'}} onClick={e=>e.stopPropagation()}>
            <Signup onSignup={(data)=>{ handleAuthSuccess(data); setShowSignup(false) }} />
          </div>
        </div>
      )}
    </div>
  )
}
