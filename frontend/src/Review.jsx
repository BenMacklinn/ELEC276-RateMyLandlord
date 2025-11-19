import React from 'react'
import { Link, useNavigate } from 'react-router-dom'
import API from './api'

export default function Review() {
  const navigate = useNavigate()
  const [landlords, setLandlords] = React.useState([])
  const [loading, setLoading] = React.useState(true)
  const [formData, setFormData] = React.useState({
    landlordId: '',
    rating: 5,
    title: '',
    review: ''
  })

  React.useEffect(() => {
    API.searchLandlords()
      .then(results => setLandlords(results))
      .catch(console.error)
      .finally(() => setLoading(false))
  }, [])

  const handleChange = (e) => {
    const { name, value } = e.target
    setFormData(prev => ({
      ...prev,
      [name]: value
    }))
  }

  const handleSubmit = async (e) => {
    e.preventDefault()
    try {
      await API.submitReview(
        formData.landlordId,
        parseInt(formData.rating),
        formData.title,
        formData.review
      )
      // Redirect to the landlord's page or home
      navigate('/')
    } catch (error) {
      console.error('Failed to submit review:', error)
      alert('Failed to submit review: ' + error.message)
    }
  }

  const pageStyle = {
    minHeight: '100vh',
    background: 'linear-gradient(135deg, rgba(96, 165, 250, 0.8) 0%, rgba(59, 130, 246, 0.8) 100%)',
    padding: '120px 24px 40px',
  }

  const contentStyle = {
    maxWidth: '800px',
    margin: '0 auto',
    backgroundColor: 'rgba(255, 255, 255, 0.15)',
    backdropFilter: 'blur(20px)',
    borderRadius: '24px',
    padding: '32px',
    border: '1px solid rgba(255, 255, 255, 0.3)',
    boxShadow: '0 8px 32px rgba(0, 0, 0, 0.1)',
  }

  const titleStyle = {
    fontSize: '2.5rem',
    fontWeight: '800',
    color: '#ffffff',
    textAlign: 'center',
    marginBottom: '32px',
    textShadow: '0 2px 4px rgba(0, 0, 0, 0.1)'
  }

  const formStyle = {
    display: 'flex',
    flexDirection: 'column',
    gap: '24px',
  }

  const fieldStyle = {
    display: 'flex',
    flexDirection: 'column',
    gap: '8px',
  }

  const labelStyle = {
    color: '#ffffff',
    fontSize: '1rem',
    fontWeight: '500',
  }

  const inputStyle = {
    padding: '12px 16px',
    backgroundColor: 'rgba(255, 255, 255, 0.9)',
    border: '1px solid rgba(255, 255, 255, 0.5)',
    borderRadius: '12px',
    fontSize: '1rem',
    outline: 'none',
    transition: 'all 0.2s ease',
  }

  const selectStyle = {
    ...inputStyle,
    appearance: 'none',
    backgroundImage: 'url("data:image/svg+xml;charset=US-ASCII,%3Csvg%20xmlns%3D%22http%3A%2F%2Fwww.w3.org%2F2000%2Fsvg%22%20width%3D%22292.4%22%20height%3D%22292.4%22%3E%3Cpath%20fill%3D%22%23007CB2%22%20d%3D%22M287%2069.4a17.6%2017.6%200%200%200-13-5.4H18.4c-5%200-9.3%201.8-12.9%205.4A17.6%2017.6%200%200%200%200%2082.2c0%205%201.8%209.3%205.4%2012.9l128%20127.9c3.6%203.6%207.8%205.4%2012.8%205.4s9.2-1.8%2012.8-5.4L287%2095c3.5-3.5%205.4-7.8%205.4-12.8%200-5-1.9-9.2-5.5-12.8z%22%2F%3E%3C%2Fsvg%3E")',
    backgroundRepeat: 'no-repeat',
    backgroundPosition: 'right 12px top 50%',
    backgroundSize: '12px auto',
    paddingRight: '40px'
  }

  const textareaStyle = {
    ...inputStyle,
    minHeight: '150px',
    resize: 'vertical'
  }

  const buttonContainerStyle = {
    display: 'flex',
    gap: '16px',
    justifyContent: 'flex-end',
    marginTop: '16px'
  }

  const buttonStyle = {
    padding: '12px 24px',
    borderRadius: '12px',
    fontSize: '1rem',
    fontWeight: '600',
    cursor: 'pointer',
    transition: 'all 0.2s ease',
    border: 'none',
  }

  const submitButtonStyle = {
    ...buttonStyle,
    backgroundColor: '#ffffff',
    color: '#3b82f6',
  }

  const cancelButtonStyle = {
    ...buttonStyle,
    backgroundColor: 'transparent',
    color: '#ffffff',
    border: '1px solid rgba(255, 255, 255, 0.5)',
  }

  if (loading) {
    return (
      <div style={pageStyle}>
        <div style={contentStyle}>
          <h1 style={titleStyle}>Loading...</h1>
        </div>
      </div>
    )
  }

  return (
    <div style={pageStyle}>
      <div style={contentStyle}>
        <h1 style={titleStyle}>Write a Review</h1>
        <form onSubmit={handleSubmit} style={formStyle}>
          <div style={fieldStyle}>
            <label style={labelStyle}>Select Landlord</label>
            <select
              name="landlordId"
              value={formData.landlordId}
              onChange={handleChange}
              style={selectStyle}
              required
            >
              <option value="">Select a landlord...</option>
              {landlords.map(ll => (
                <option key={ll.landlord_id} value={ll.landlord_id}>
                  {ll.name}
                </option>
              ))}
            </select>
          </div>

          <div style={fieldStyle}>
            <label style={labelStyle}>Rating</label>
            <select
              name="rating"
              value={formData.rating}
              onChange={handleChange}
              style={selectStyle}
              required
            >
              <option value="5">⭐⭐⭐⭐⭐ (5/5)</option>
              <option value="4">⭐⭐⭐⭐ (4/5)</option>
              <option value="3">⭐⭐⭐ (3/5)</option>
              <option value="2">⭐⭐ (2/5)</option>
              <option value="1">⭐ (1/5)</option>
            </select>
          </div>

          <div style={fieldStyle}>
            <label style={labelStyle}>Review Title</label>
            <input
              type="text"
              name="title"
              value={formData.title}
              onChange={handleChange}
              style={inputStyle}
              placeholder="Enter a title for your review"
              required
            />
          </div>

          <div style={fieldStyle}>
            <label style={labelStyle}>Your Experience</label>
            <textarea
              name="review"
              value={formData.review}
              onChange={handleChange}
              style={textareaStyle}
              placeholder="Share details of your experience with this landlord..."
              required
            />
          </div>

          <div style={buttonContainerStyle}>
            <button
              type="button"
              onClick={() => navigate('/')}
              style={cancelButtonStyle}
            >
              Cancel
            </button>
            <button
              type="submit"
              style={submitButtonStyle}
            >
              Submit Review
            </button>
          </div>
        </form>
      </div>
    </div>
  )
}