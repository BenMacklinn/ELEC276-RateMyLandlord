const API = {
  async login(email, password) {
    const res = await fetch('/api/auth/login', {
      method: 'POST',
      headers: {'Content-Type':'application/json'},
      body: JSON.stringify({email, password})
    })
    const j = await res.json().catch(()=>({}))
    if(!res.ok) throw new Error(j.error || 'Login failed')
    return j
  },
  async signup(name, email, password) {
    const res = await fetch('/api/auth/signup', {
      method: 'POST',
      headers: {'Content-Type':'application/json'},
      body: JSON.stringify({name, email, password})
    })
    const j = await res.json().catch(()=>({}))
    if(!res.ok) throw new Error(j.error || 'Signup failed')
    return j
  },
  async requestVerification(email) {
    const res = await fetch('/api/auth/request-verification', {
      method: 'POST',
      headers: {'Content-Type':'application/json'},
      body: JSON.stringify({email})
    })
    const j = await res.json().catch(()=>({}))
    if(!res.ok) throw new Error(j.error || 'Failed to send verification code')
    return j
  },
  async verifyEmailCode(email, code) {
    const res = await fetch('/api/auth/verify-code', {
      method: 'POST',
      headers: {'Content-Type':'application/json'},
      body: JSON.stringify({email, code})
    })
    const j = await res.json().catch(()=>({}))
    if(!res.ok) throw new Error(j.error || 'Invalid verification code')
    return j
  },
  async me(token) {
    const res = await fetch('/api/users/me', {
      headers: { 'Authorization': `Bearer ${token}` }
    })
    const j = await res.json().catch(()=>({}))
    if(!res.ok) throw new Error(j.error || 'Not authorized')
    return j
  },
  async searchLandlords(name) {
    const u = new URL('/api/landlords/search', window.location.origin)
    if(name) u.searchParams.set('name', name)
    const res = await fetch(u.toString())
    const j = await res.json().catch(()=>({}))
    if(!res.ok) throw new Error(j.error || 'Search failed')
    return j.results || []
  },
  async getStats() {
    const res = await fetch('/api/landlords/stats')
    const j = await res.json().catch(()=>({}))
    if(!res.ok) throw new Error(j.error || 'Failed to fetch stats')
    return j
  },
  async getLeaderboard() {
    const res = await fetch('/api/landlords/leaderboard')
    const j = await res.json().catch(()=>({}))
    if(!res.ok) throw new Error(j.error || 'Failed to fetch leaderboard')
    return j.leaderboard || []
  },
  async submitReview(landlordId, rating, title, review) {
    const res = await fetch('/api/reviews/submit', {
      method: 'POST',
      headers: {'Content-Type': 'application/json'},
      body: JSON.stringify({landlord_id: landlordId, rating, title, review})
    })
    const j = await res.json().catch(()=>({}))
    if(!res.ok) throw new Error(j.error || 'Failed to submit review')
    return j
  },
  async getLandlordReviews(landlordId) {
    const res = await fetch(`/api/reviews/landlord/${landlordId}`)
    const j = await res.json().catch(()=>({}))
    if(!res.ok) throw new Error(j.error || 'Failed to fetch reviews')
    return j.reviews || []
  }
}
export default API
