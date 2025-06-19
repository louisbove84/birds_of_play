// Switch to birds_of_play database
use birds_of_play;

// Show collections
show collections;

// Count total tracked objects
db.motion_tracking_data.count();

// Find objects with high confidence (> 0.8)
db.motion_tracking_data.find({
    confidence: { $gt: 0.8 }
}).pretty();

// Find objects tracked in the last hour
db.motion_tracking_data.find({
    last_seen: { 
        $gt: new Date(Date.now() - 1000 * 60 * 60)
    }
}).pretty();

// Find objects that moved more than 100 pixels
db.motion_tracking_data.find({
    $where: "this.trajectory.length > 10"
}).pretty();

// Get trajectory statistics
db.motion_tracking_data.aggregate([
    {
        $project: {
            uuid: 1,
            trajectory_length: { $size: "$trajectory" },
            tracking_duration_ms: {
                $subtract: ["$last_seen", "$first_seen"]
            }
        }
    }
]);

// Find objects that appeared in a specific region
db.motion_tracking_data.find({
    "initial_bounds.x": { $gt: 100, $lt: 500 },
    "initial_bounds.y": { $gt: 100, $lt: 500 }
}).pretty();

// Get the image for a specific tracking ID
// Replace YOUR_UUID with an actual UUID from the tracking data
db.motion_tracking_images.findOne({
    uuid: "YOUR_UUID"
});

// Delete old data (older than 7 days)
db.motion_tracking_data.deleteMany({
    last_seen: {
        $lt: new Date(Date.now() - 1000 * 60 * 60 * 24 * 7)
    }
});

// Create indexes for better query performance
db.motion_tracking_data.createIndex({ "uuid": 1 });
db.motion_tracking_data.createIndex({ "first_seen": 1 });
db.motion_tracking_data.createIndex({ "last_seen": 1 });
db.motion_tracking_data.createIndex({ "confidence": 1 });
db.motion_tracking_images.createIndex({ "uuid": 1 }); 