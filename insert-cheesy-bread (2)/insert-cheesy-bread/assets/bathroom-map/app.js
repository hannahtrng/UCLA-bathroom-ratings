// Initialize map centered on UCLA
const uclaCenter = [34.0689, -118.4452];
const map = L.map('map').setView(uclaCenter, 15);

// Add tile layer (OpenStreetMap)
L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {
    attribution: '© OpenStreetMap contributors',
    maxZoom: 19,
}).addTo(map);

let heatLayer = null;
let markers = [];
let allBathrooms = []; // Store all bathrooms globally for distance calculations
let routingControl = null; // Store routing control for route display
let currentUserLocation = null; // Store current user location

// Show loading indicator
function showLoading() {
    document.getElementById('loading').classList.add('show');
}

function hideLoading() {
    document.getElementById('loading').classList.remove('show');
}

// Fetch all bathroom data from API
async function loadBathrooms() {
    showLoading();

    try {
        // Step 1: Get list of all bathroom IDs
        const idsResponse = await fetch('/api/bathrooms');

        if (!idsResponse.ok) {
            throw new Error(
                `Failed to fetch bathroom IDs: ${idsResponse.status}`
            );
        }

        const ids = await idsResponse.json();

        if (!Array.isArray(ids) || ids.length === 0) {
            console.log('No bathrooms found');
            hideLoading();
            return;
        }

        console.log(`Found ${ids.length} bathrooms`);

        // Step 2: Fetch each bathroom entity
        const bathroomPromises = ids.map(async (id) => {
            try {
                const response = await fetch(`/api/bathrooms/${id}`);
                if (!response.ok) {
                    console.warn(`Failed to fetch bathroom ${id}`);
                    return null;
                }
                const data = await response.json();
                return {
                    id: id,
                    ...data,
                };
            } catch (error) {
                console.warn(`Error fetching bathroom ${id}:`, error);
                return null;
            }
        });

        const bathrooms = (await Promise.all(bathroomPromises)).filter(
            (bathroom) =>
                bathroom !== null &&
                bathroom.latitude !== undefined &&
                bathroom.longitude !== undefined
        );

        // Store bathrooms globally for distance calculations
        allBathrooms = bathrooms;

        console.log(`Loaded ${bathrooms.length} valid bathrooms`);

        // Step 3: Create heatmap data points
        // Format: [lat, lng, intensity]
        // Intensity is normalized rating (0-1 scale, assuming 0-5 rating system)
        const heatmapData = bathrooms.map((bathroom) => {
            const lat = parseFloat(bathroom.latitude);
            const lng = parseFloat(bathroom.longitude);
            // Normalize rating to 0-1 scale (assuming 0-5 rating)
            const intensity = Math.min(
                1.0,
                Math.max(0.0, parseFloat(bathroom.rating || 0) / 5.0)
            );

            return [lat, lng, intensity];
        });

        // Step 4: Remove existing heat layer if present
        if (heatLayer) {
            map.removeLayer(heatLayer);
        }

        // Step 5: Add new heatmap layer
        heatLayer = L.heatLayer(heatmapData, {
            radius: 30,
            blur: 20,
            maxZoom: 17,
            minOpacity: 0.5,
            gradient: {
                0.0: 'blue', // Low ratings (cold)
                0.3: 'cyan', // Below average
                0.5: 'yellow', // Average
                0.7: 'orange', // Above average
                1.0: 'red', // High ratings (hot)
            },
        }).addTo(map);

        // Step 6: Add markers for each bathroom (optional - can be toggled)
        // Clear existing markers
        markers.forEach((marker) => map.removeLayer(marker));
        markers = [];

        bathrooms.forEach((bathroom) => {
            const lat = parseFloat(bathroom.latitude);
            const lng = parseFloat(bathroom.longitude);
            const rating = parseFloat(bathroom.rating || 0);
            const numRatings = parseInt(bathroom.num_ratings || 0);

            // Choose marker color based on rating
            let markerColor = 'gray';
            if (rating >= 4.0) markerColor = 'green';
            else if (rating >= 3.0) markerColor = 'yellow';
            else if (rating >= 2.0) markerColor = 'orange';
            else markerColor = 'red';

            const marker = L.circleMarker([lat, lng], {
                radius: 8,
                fillColor: markerColor,
                color: '#fff',
                weight: 2,
                opacity: 0.8,
                fillOpacity: 0.7,
            }).addTo(map);

            // Create popup content
            const popupContent = `
                <b>${bathroom.name || 'Unnamed Bathroom'}</b>
                <div class="rating">⭐ ${rating.toFixed(1)}/5.0</div>
                <div class="details">
                    ${
                        bathroom.building
                            ? `Building: ${bathroom.building}<br>`
                            : ''
                    }
                    ${bathroom.floor ? `Floor: ${bathroom.floor}<br>` : ''}
                    ${numRatings} ${numRatings === 1 ? 'rating' : 'ratings'}
                </div>
            `;

            marker.bindPopup(popupContent);
            markers.push(marker);
        });

        hideLoading();
    } catch (error) {
        console.error('Error loading bathrooms:', error);
        hideLoading();
        alert(
            'Failed to load bathroom data. Make sure the server is running and bathrooms are populated.'
        );
    }
}

// Load bathrooms when page loads
loadBathrooms();

// Optional: Add refresh button functionality
// You can add a refresh button to the header if needed
function refreshData() {
    loadBathrooms();
}

// Make refreshData available globally for potential button
window.refreshBathrooms = refreshData;

// Calculate distance between two coordinates using Haversine formula
// Returns distance in kilometers
function calculateDistance(lat1, lon1, lat2, lon2) {
    const R = 6371; // Earth's radius in kilometers
    const dLat = ((lat2 - lat1) * Math.PI) / 180;
    const dLon = ((lon2 - lon1) * Math.PI) / 180;
    const a =
        Math.sin(dLat / 2) * Math.sin(dLat / 2) +
        Math.cos((lat1 * Math.PI) / 180) *
            Math.cos((lat2 * Math.PI) / 180) *
            Math.sin(dLon / 2) *
            Math.sin(dLon / 2);
    const c = 2 * Math.atan2(Math.sqrt(a), Math.sqrt(1 - a));
    return R * c; // Distance in kilometers
}

// Convert kilometers to miles
function kmToMiles(km) {
    return (km * 0.621371).toFixed(2);
}

// Find closest bathroom(s) to user location
function findClosestBathrooms(userLat, userLng, count = 5) {
    if (allBathrooms.length === 0) {
        return [];
    }

    // Calculate distance for each bathroom
    const bathroomsWithDistance = allBathrooms.map((bathroom) => {
        const distance = calculateDistance(
            userLat,
            userLng,
            parseFloat(bathroom.latitude),
            parseFloat(bathroom.longitude)
        );
        return {
            ...bathroom,
            distance: distance, // in km
            distanceMiles: kmToMiles(distance), // in miles
        };
    });

    // Sort by distance and return top N
    return bathroomsWithDistance
        .sort((a, b) => a.distance - b.distance)
        .slice(0, count);
}

// Find and highlight the closest bathroom
function findNearestBathroom() {
    if (allBathrooms.length === 0) {
        alert('No bathrooms loaded. Please wait for data to load.');
        return;
    }

    // Check if geolocation is supported
    if (!navigator.geolocation) {
        alert('Geolocation is not supported by your browser.');
        return;
    }

    showLoading();

    // Request high accuracy location
    const geoOptions = {
        enableHighAccuracy: true, // Request GPS if available
        timeout: 10000, // 10 second timeout
        maximumAge: 0, // Don't use cached position
    };

    navigator.geolocation.getCurrentPosition(
        (position) => {
            const userLat = position.coords.latitude;
            const userLng = position.coords.longitude;
            const accuracy = position.coords.accuracy; // Accuracy in meters

            // Store user location globally
            currentUserLocation = {
                lat: userLat,
                lng: userLng,
                accuracy: accuracy,
            };

            console.log(
                `Location obtained: ${userLat}, ${userLng} (accuracy: ${accuracy.toFixed(
                    0
                )}m)`
            );

            // Find closest bathroom
            const closest = findClosestBathrooms(userLat, userLng, 1)[0];

            if (!closest) {
                hideLoading();
                alert('No bathrooms found.');
                return;
            }

            // Center map to show both user location and closest bathroom
            const bounds = L.latLngBounds(
                [userLat, userLng],
                [closest.latitude, closest.longitude]
            );
            map.fitBounds(bounds, { padding: [50, 50] });

            // Add user location marker
            addUserLocationMarker(userLat, userLng, accuracy);

            // Find and open the marker popup
            const marker = markers.find((m) => {
                const latLng = m.getLatLng();
                return (
                    Math.abs(latLng.lat - closest.latitude) < 0.0001 &&
                    Math.abs(latLng.lng - closest.longitude) < 0.0001
                );
            });

            if (marker) {
                marker.openPopup();
            }

            // Update closest bathrooms list
            updateClosestBathroomsList(userLat, userLng);

            // Show route button
            showRouteButton(closest);

            hideLoading();
        },
        (error) => {
            hideLoading();
            console.error('Geolocation error:', error);
            let errorMsg = 'Unable to get your location. ';
            if (error.code === 1) {
                errorMsg += 'Please allow location access and try again.';
            } else if (error.code === 2) {
                errorMsg +=
                    'Location unavailable. Please check your connection.';
            } else if (error.code === 3) {
                errorMsg += 'Location request timed out. Please try again.';
            } else {
                errorMsg += 'Please enable location services and try again.';
            }
            alert(errorMsg);
        },
        geoOptions
    );
}

// Update the closest bathrooms list sidebar
function updateClosestBathroomsList(userLat, userLng) {
    const closest = findClosestBathrooms(userLat, userLng, 5);
    const listContainer = document.getElementById('closest-list');
    const sidebar = document.getElementById('closest-sidebar');

    // Show sidebar
    sidebar.classList.add('show');

    if (closest.length === 0) {
        listContainer.innerHTML = '<p>No bathrooms found nearby.</p>';
        return;
    }

    listContainer.innerHTML = '<h3>📍 Nearest Bathrooms</h3>';

    closest.forEach((bathroom, index) => {
        const rating = parseFloat(bathroom.rating || 0);
        const item = document.createElement('div');
        item.className = 'closest-item';
        item.innerHTML = `
            <div class="closest-number">${index + 1}</div>
            <div class="closest-info">
                <div class="closest-name">${
                    bathroom.name || 'Unnamed Bathroom'
                }</div>
                <div class="closest-details">
                    <span class="closest-rating">⭐ ${rating.toFixed(1)}</span>
                    <span class="closest-distance">${
                        bathroom.distanceMiles
                    } mi</span>
                </div>
            </div>
        `;

        // Create route button
        const routeBtn = document.createElement('button');
        routeBtn.className = 'route-btn';
        routeBtn.textContent = '🧭';
        routeBtn.title = 'Show route';
        routeBtn.onclick = (e) => {
            e.stopPropagation();
            showRouteToBathroom(bathroom);
        };
        item.appendChild(routeBtn);

        // Add click handler to center map on this bathroom
        item.addEventListener('click', () => {
            map.setView([bathroom.latitude, bathroom.longitude], 18);
            const marker = markers.find((m) => {
                const latLng = m.getLatLng();
                return (
                    Math.abs(latLng.lat - bathroom.latitude) < 0.0001 &&
                    Math.abs(latLng.lng - bathroom.longitude) < 0.0001
                );
            });
            if (marker) {
                marker.openPopup();
            }
        });

        listContainer.appendChild(item);
    });
}

// Add user location marker
let userLocationMarker = null;
function addUserLocationMarker(lat, lng, accuracy) {
    // Remove existing user location marker
    if (userLocationMarker) {
        map.removeLayer(userLocationMarker);
    }

    // Add user location marker with accuracy circle
    userLocationMarker = L.marker([lat, lng], {
        icon: L.divIcon({
            className: 'user-location-marker',
            html: '<div class="user-location-pin">📍</div>',
            iconSize: [30, 30],
            iconAnchor: [15, 30],
        }),
    }).addTo(map);

    // Add accuracy circle
    const accuracyCircle = L.circle([lat, lng], {
        radius: accuracy,
        color: '#0066cc',
        fillColor: '#0066cc',
        fillOpacity: 0.1,
        weight: 2,
        dashArray: '5, 5',
    }).addTo(map);

    userLocationMarker.bindPopup(
        `Your location<br>Accuracy: ${accuracy.toFixed(0)} meters`
    );
}

// Show route to bathroom
function showRouteToBathroom(bathroom) {
    if (!currentUserLocation) {
        alert('Please find your location first by clicking "Find Nearest"');
        return;
    }

    // Remove existing route if present
    if (routingControl) {
        map.removeControl(routingControl);
        routingControl = null;
    }

    // Create route from user location to bathroom
    routingControl = L.Routing.control({
        waypoints: [
            L.latLng(currentUserLocation.lat, currentUserLocation.lng),
            L.latLng(bathroom.latitude, bathroom.longitude),
        ],
        routeWhileDragging: false,
        showAlternatives: false,
        addWaypoints: false,
        createMarker: function (i, waypoint) {
            // Don't create markers for waypoints (we already have them)
            return null;
        },
        lineOptions: {
            styles: [
                {
                    color: '#0066cc',
                    weight: 5,
                    opacity: 0.8,
                },
            ],
        },
        router: L.Routing.osrmv1({
            serviceUrl: 'https://router.project-osrm.org/route/v1',
            profile: 'foot', // Use walking profile
        }),
    }).addTo(map);

    // Center map to show entire route
    const bounds = L.latLngBounds(
        [currentUserLocation.lat, currentUserLocation.lng],
        [bathroom.latitude, bathroom.longitude]
    );
    map.fitBounds(bounds, { padding: [50, 50] });
}

// Make showRouteToBathroom available globally for onclick handlers
window.showRouteToBathroom = showRouteToBathroom;

// Show route button in closest bathrooms list
function showRouteButton(bathroom) {
    // This will be called when updating the list, button added in list items
}

// Clear route
function clearRoute() {
    if (routingControl) {
        map.removeControl(routingControl);
        routingControl = null;
    }
}

// Initialize closest bathrooms feature
function initClosestBathroomsFeature() {
    // Add button to header
    const button = document.createElement('button');
    button.id = 'find-nearest-btn';
    button.textContent = '📍 Find Nearest';
    button.onclick = findNearestBathroom;
    document.getElementById('header').appendChild(button);

    // Add clear route button
    const clearRouteBtn = document.createElement('button');
    clearRouteBtn.id = 'clear-route-btn';
    clearRouteBtn.textContent = '🗺️ Clear Route';
    clearRouteBtn.onclick = clearRoute;
    clearRouteBtn.style.marginTop = '8px';
    clearRouteBtn.style.background = '#666';
    clearRouteBtn.onmouseover = function () {
        this.style.background = '#555';
    };
    clearRouteBtn.onmouseout = function () {
        this.style.background = '#666';
    };
    document.getElementById('header').appendChild(clearRouteBtn);
}

// Initialize when page loads
initClosestBathroomsFeature();
