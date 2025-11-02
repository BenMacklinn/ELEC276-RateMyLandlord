import React from 'react'
import API from './api'

function ReviewList({ landlordId }) {
  const [reviews, setReviews] = React.useState([])
  const [loading, setLoading] = React.useState(true)

  React.useEffect(() => {
    let ok = true
    API.getLandlordReviews(landlordId)
      .then(reviews => { 
        if (ok) { 
          console.log('Received reviews:', reviews) // Debug log
          setReviews(reviews)
          setLoading(false) 
        }
      })
      .catch((error) => { 
        console.error('Error fetching reviews:', error) // Debug log
        if (ok) setLoading(false) 
      })
    return () => { ok = false }
  }, [landlordId])

  if (loading) return <div>Loading reviews...</div>
  if (!reviews.length) return <div>No reviews yet</div>

  return (
    <div style={{ marginTop: '12px' }}>
      {reviews.map(review => (
        <div key={review.id} style={{ 
          border: '1px solid #ddd', 
          borderRadius: '4px', 
          padding: '12px',
          marginBottom: '8px' 
        }}>
          <div style={{ display: 'flex', justifyContent: 'space-between' }}>
            <div style={{ fontWeight: 'bold' }}>{review.title}</div>
            <div>Rating: {review.rating}/5</div>
          </div>
          <div style={{ marginTop: '8px' }}>{review.review}</div>
          <div style={{ fontSize: '0.8em', color: '#666', marginTop: '8px' }}>
            {new Date(review.created_at).toLocaleDateString()}
          </div>
        </div>
      ))}
    </div>
  )
}

export default function Profile({token, onLogout}){
  const [me, setMe] = React.useState(null)
  const [err, setErr] = React.useState('')
  const [landlords, setLandlords] = React.useState([])

  React.useEffect(()=>{
    let ok = true
    API.me(token).then(d => { if(ok) setMe(d) }).catch(e=> setErr(e.message))
    API.searchLandlords().then(d => { if(ok) setLandlords(d) })
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
          
          <h3 style={{marginTop: '24px'}}>Property Listings</h3>
          <div style={{display: 'flex', flexDirection: 'column', gap: '20px', marginTop: '16px'}}>
            {landlords.map(landlord => (
              <div key={landlord.landlord_id} style={{
                backgroundColor: 'rgba(255, 255, 255, 0.8)',
                borderRadius: '8px',
                padding: '20px',
                boxShadow: '0 2px 4px rgba(0, 0, 0, 0.1)'
              }}>
                <h4 style={{margin: '0 0 16px', fontSize: '1.25rem', color: '#2d3748'}}>
                  {landlord.name}
                </h4>
                <div style={{marginBottom: '20px'}}>
                  {landlord.properties.map(property => (
                    <div key={property.property_id} style={{
                      border: '1px solid #e5e7eb',
                      borderRadius: '6px',
                      padding: '16px',
                      marginBottom: '12px'
                    }}>
                      <div style={{fontWeight: '500', marginBottom: '8px'}}>
                        {property.address.street}, {property.address.city}, {property.address.state}
                      </div>
                      {property.unit_details.map((unit, index) => (
                        <div key={index} style={{marginBottom: '8px', paddingLeft: '12px'}}>
                          <div style={{fontWeight: '500'}}>{unit.unit_number}:</div>
                          <div style={{color: '#4a5568'}}>
                            {unit.bedrooms} bedroom, {unit.bathrooms} bathroom - ${unit.rent}/month
                          </div>
                        </div>
                      ))}
                    </div>
                  ))}
                </div>
                <div style={{borderTop: '2px solid #e5e7eb', paddingTop: '16px'}}>
                  <h5 style={{margin: '0 0 12px', fontSize: '1.1rem', color: '#2d3748'}}>
                    Reviews for {landlord.name}
                  </h5>
                  <ReviewList landlordId={landlord.landlord_id} />
                </div>
              </div>
            ))}
          </div>
        </div>
      ) : <div>Loading…</div>}
      <button style={{marginTop:12}} onClick={onLogout}>Log out</button>
    </div>
  )
}
