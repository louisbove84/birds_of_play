# Interface Specification Document
## birds_of_play: Real-Time Object Tracking System

**Document Version:** 1.0  
**Date:** 2024  
**Standards Compliance:** IEEE 1016-2009 (Software Design Descriptions)

---

## 1. INTERFACE OVERVIEW

This document defines all external and internal interfaces for the birds_of_play tracking system, ensuring interoperability and maintainability.

### 1.1 Interface Categories
- **Sensor Interfaces:** Hardware sensor communication protocols
- **Network Interfaces:** External system communication APIs  
- **Internal Interfaces:** Component-to-component communication
- **Configuration Interfaces:** System configuration and management
- **Monitoring Interfaces:** System health and performance metrics

---

## 2. SENSOR INTERFACES

### 2.1 Radar Interface Protocol

#### 2.1.1 Message Format
```protobuf
syntax = "proto3";

message RadarMeasurement {
    double timestamp = 1;           // Unix timestamp (seconds)
    double range = 2;              // Distance in meters
    double bearing = 3;            // Angle in radians
    double elevation = 4;          // Elevation angle in radians
    double range_rate = 5;         // Radial velocity in m/s
    double rcs = 6;               // Radar cross section in dBsm
    double snr = 7;               // Signal-to-noise ratio in dB
    uint32 detection_id = 8;       // Unique detection identifier
}

message RadarFrame {
    double frame_timestamp = 1;
    uint32 frame_number = 2;
    repeated RadarMeasurement detections = 3;
    RadarStatus status = 4;
}

message RadarStatus {
    bool is_operational = 1;
    double temperature = 2;        // Celsius
    double power_level = 3;        // Watts
    string error_message = 4;
}
```

#### 2.1.2 Communication Protocol
- **Transport:** UDP multicast on 239.255.0.1:5000
- **Encoding:** Protocol Buffers binary format
- **Rate:** 20 Hz nominal, up to 50 Hz burst
- **Timeout:** 100ms maximum between frames

### 2.2 Camera Interface Protocol

#### 2.2.1 Detection Message Format
```protobuf
message CameraDetection {
    double timestamp = 1;
    BoundingBox bbox = 2;
    double confidence = 3;          // 0.0 to 1.0
    string object_class = 4;        // "person", "vehicle", etc.
    Point2D centroid = 5;
    repeated Point2D keypoints = 6; // Optional object keypoints
}

message BoundingBox {
    double x = 1;                  // Top-left x (normalized 0-1)
    double y = 2;                  // Top-left y (normalized 0-1)
    double width = 3;              // Width (normalized 0-1)
    double height = 4;             // Height (normalized 0-1)
}

message Point2D {
    double x = 1;                  // Normalized coordinates 0-1
    double y = 2;
}

message CameraFrame {
    double frame_timestamp = 1;
    uint32 frame_number = 2;
    repeated CameraDetection detections = 3;
    CameraStatus status = 4;
}
```

#### 2.2.2 Video Stream Interface
- **Protocol:** RTSP (Real Time Streaming Protocol)
- **Format:** H.264 encoded, 1920x1080 @ 30fps
- **URL:** `rtsp://camera_ip:554/stream1`
- **Authentication:** Basic HTTP authentication

### 2.3 LiDAR Interface Protocol

#### 2.3.1 Point Cloud Format
```protobuf
message LiDARPoint {
    float x = 1;                   // Meters
    float y = 2;                   // Meters  
    float z = 3;                   // Meters
    float intensity = 4;           // Reflection intensity 0-255
    double timestamp = 5;          // Point acquisition time
}

message LiDARScan {
    double scan_timestamp = 1;
    uint32 scan_number = 2;
    repeated LiDARPoint points = 3;
    LiDARStatus status = 4;
}

message LiDARStatus {
    bool is_spinning = 1;
    double rpm = 2;                // Rotations per minute
    double temperature = 3;        // Celsius
    uint32 error_code = 4;
}
```

---

## 3. NETWORK INTERFACES

### 3.1 REST API Interface

#### 3.1.1 Track Query API
```yaml
# GET /api/v1/tracks
responses:
  200:
    description: List of active tracks
    content:
      application/json:
        schema:
          type: object
          properties:
            tracks:
              type: array
              items:
                $ref: '#/components/schemas/Track'
            metadata:
              $ref: '#/components/schemas/QueryMetadata'

# GET /api/v1/tracks/{id}
parameters:
  - name: id
    in: path
    required: true
    schema:
      type: integer
      minimum: 1
responses:
  200:
    description: Track details
    content:
      application/json:
        schema:
          $ref: '#/components/schemas/Track'
  404:
    description: Track not found

components:
  schemas:
    Track:
      type: object
      required: [id, position, velocity, timestamp]
      properties:
        id:
          type: integer
          description: Unique track identifier
        position:
          $ref: '#/components/schemas/Position3D'
        velocity:
          $ref: '#/components/schemas/Velocity3D'
        acceleration:
          $ref: '#/components/schemas/Acceleration3D'
        covariance:
          $ref: '#/components/schemas/CovarianceMatrix'
        confidence:
          type: number
          minimum: 0.0
          maximum: 1.0
        object_class:
          type: string
          enum: [unknown, person, vehicle, aircraft]
        timestamp:
          type: string
          format: date-time
        last_seen:
          type: string
          format: date-time
          
    Position3D:
      type: object
      required: [x, y, z]
      properties:
        x:
          type: number
          description: X coordinate in meters
        y:
          type: number
          description: Y coordinate in meters
        z:
          type: number
          description: Z coordinate in meters
```

#### 3.1.2 Configuration API
```yaml
# PUT /api/v1/config/processing
requestBody:
  required: true
  content:
    application/json:
      schema:
        $ref: '#/components/schemas/ProcessingConfig'
responses:
  200:
    description: Configuration updated successfully
  400:
    description: Invalid configuration
    content:
      application/json:
        schema:
          $ref: '#/components/schemas/ErrorResponse'

# GET /api/v1/status
responses:
  200:
    description: System status
    content:
      application/json:
        schema:
          type: object
          properties:
            system_state:
              type: string
              enum: [normal, degraded, critical, shutdown]
            uptime_seconds:
              type: integer
            active_tracks:
              type: integer
            processing_rate:
              type: number
              description: Frames per second
            memory_usage:
              type: object
              properties:
                current_mb:
                  type: number
                peak_mb:
                  type: number
                limit_mb:
                  type: number
```

### 3.2 WebSocket Interface

#### 3.2.1 Real-Time Track Updates
```javascript
// Connection
const ws = new WebSocket('ws://localhost:8081/tracks');

// Message Types
const MessageType = {
    TRACK_UPDATE: 'track_update',
    TRACK_CREATED: 'track_created', 
    TRACK_DELETED: 'track_deleted',
    SYSTEM_STATUS: 'system_status',
    ERROR: 'error'
};

// Track Update Message
{
    "type": "track_update",
    "timestamp": "2024-03-15T10:30:45.123Z",
    "data": {
        "track_id": 12345,
        "position": {
            "x": 150.25,
            "y": 75.80,
            "z": 2.10
        },
        "velocity": {
            "x": 5.2,
            "y": -1.8,
            "z": 0.0
        },
        "confidence": 0.95,
        "object_class": "vehicle"
    }
}

// System Status Message
{
    "type": "system_status",
    "timestamp": "2024-03-15T10:30:45.123Z",
    "data": {
        "state": "normal",
        "active_tracks": 23,
        "processing_latency_ms": 3.2,
        "memory_usage_mb": 42.1
    }
}
```

#### 3.2.2 Client Subscription Management
```javascript
// Subscribe to specific tracks
ws.send(JSON.stringify({
    "type": "subscribe",
    "data": {
        "track_ids": [12345, 67890],
        "update_rate_hz": 10
    }
}));

// Subscribe to area of interest
ws.send(JSON.stringify({
    "type": "subscribe_area",
    "data": {
        "bounds": {
            "min_x": 0, "max_x": 200,
            "min_y": 0, "max_y": 200
        },
        "update_rate_hz": 20
    }
}));
```

---

## 4. INTERNAL INTERFACES

### 4.1 Component Communication

#### 4.1.1 Message Bus Interface
```cpp
class MessageBus {
public:
    template<typename T>
    using MessageHandler = std::function<void(const T&)>;
    
    template<typename T>
    void publish(const T& message);
    
    template<typename T>
    void subscribe(const MessageHandler<T>& handler);
    
    template<typename T>
    void unsubscribe(const MessageHandler<T>& handler);
    
private:
    template<typename T>
    std::vector<MessageHandler<T>> handlers_;
    std::mutex handlers_mutex_;
};

// Message Types
struct SensorDataMessage {
    SensorType sensor_type;
    std::vector<uint8_t> data;
    std::chrono::time_point<std::chrono::steady_clock> timestamp;
};

struct TrackUpdateMessage {
    uint32_t track_id;
    TrackState previous_state;
    TrackState new_state;
    Eigen::VectorXd position;
    Eigen::VectorXd velocity;
    Eigen::MatrixXd covariance;
};

struct SystemEventMessage {
    EventType type;
    std::string component;
    std::string description;
    Severity severity;
    std::chrono::time_point<std::chrono::steady_clock> timestamp;
};
```

#### 4.1.2 Service Discovery Interface
```cpp
class ServiceRegistry {
public:
    struct ServiceInfo {
        std::string name;
        std::string version;
        std::string endpoint;
        std::map<std::string, std::string> metadata;
        std::chrono::time_point<std::chrono::steady_clock> last_heartbeat;
    };
    
    void registerService(const ServiceInfo& info);
    void unregisterService(const std::string& name);
    std::optional<ServiceInfo> findService(const std::string& name);
    std::vector<ServiceInfo> listServices();
    void updateHeartbeat(const std::string& name);
    
private:
    std::unordered_map<std::string, ServiceInfo> services_;
    std::shared_mutex services_mutex_;
    std::thread cleanup_thread_;
};
```

### 4.2 Data Flow Interfaces

#### 4.2.1 Processing Pipeline
```cpp
template<typename InputType, typename OutputType>
class ProcessingStage {
public:
    virtual ~ProcessingStage() = default;
    virtual OutputType process(const InputType& input) = 0;
    virtual bool isHealthy() const = 0;
    virtual std::string getName() const = 0;
};

class ProcessingPipeline {
public:
    template<typename InputType, typename OutputType>
    void addStage(std::unique_ptr<ProcessingStage<InputType, OutputType>> stage);
    
    void processAsync(const SensorData& input);
    void setErrorHandler(std::function<void(const std::exception&)> handler);
    
private:
    std::vector<std::unique_ptr<ProcessingStageBase>> stages_;
    std::function<void(const std::exception&)> error_handler_;
    ThreadPool thread_pool_;
};
```

---

## 5. CONFIGURATION INTERFACES

### 5.1 Configuration Schema

#### 5.1.1 System Configuration
```yaml
# system_config.yaml
system:
  name: "birds_of_play"
  version: "1.0.0"
  log_level: "INFO"
  
processing:
  max_processing_time_ms: 5
  max_memory_mb: 50
  thread_pool_size: 4
  enable_performance_monitoring: true
  
sensors:
  radar:
    enabled: true
    ip_address: "192.168.1.100"
    port: 5000
    timeout_ms: 100
    max_range_m: 1000
    
  camera:
    enabled: true
    rtsp_url: "rtsp://192.168.1.101:554/stream1"
    username: "admin"
    password: "password"
    detection_threshold: 0.7
    
  lidar:
    enabled: false
    ip_address: "192.168.1.102"
    port: 2368
    
tracking:
  kalman_filter:
    process_noise: 0.1
    measurement_noise: 1.0
    initial_covariance: 10.0
    
  association:
    max_distance: 50.0
    gate_threshold: 9.21  # Chi-squared 95% confidence
    
  track_management:
    min_hits_to_confirm: 3
    max_misses_to_delete: 5
    max_coasting_time_s: 5.0
    
network:
  rest_api:
    enabled: true
    port: 8080
    cors_enabled: true
    
  websocket:
    enabled: true
    port: 8081
    max_connections: 100
    
safety:
  watchdog_timeout_ms: 1000
  enable_safe_shutdown: true
  max_consecutive_errors: 5
```

#### 5.1.2 Configuration Validation
```cpp
class ConfigValidator {
public:
    struct ValidationResult {
        bool is_valid;
        std::vector<std::string> errors;
        std::vector<std::string> warnings;
    };
    
    ValidationResult validateSystemConfig(const SystemConfig& config);
    ValidationResult validateProcessingConfig(const ProcessingConfig& config);
    ValidationResult validateSensorConfig(const SensorConfig& config);
    
private:
    bool validateRange(double value, double min, double max, const std::string& param_name);
    bool validateIPAddress(const std::string& ip);
    bool validatePort(uint16_t port);
};
```

---

## 6. MONITORING INTERFACES

### 6.1 Metrics Interface

#### 6.1.1 Performance Metrics
```cpp
class MetricsCollector {
public:
    struct PerformanceMetrics {
        // Timing metrics
        std::chrono::microseconds avg_processing_time;
        std::chrono::microseconds max_processing_time;
        std::chrono::microseconds min_processing_time;
        
        // Throughput metrics
        double frames_per_second;
        double tracks_per_second;
        
        // Resource metrics
        size_t memory_usage_bytes;
        double cpu_utilization_percent;
        
        // Quality metrics
        double track_accuracy;
        double false_positive_rate;
        double false_negative_rate;
    };
    
    void recordProcessingTime(std::chrono::microseconds time);
    void recordMemoryUsage(size_t bytes);
    void recordTrackAccuracy(double accuracy);
    
    PerformanceMetrics getMetrics() const;
    void resetMetrics();
    
private:
    mutable std::shared_mutex metrics_mutex_;
    PerformanceMetrics current_metrics_;
    std::deque<std::chrono::microseconds> processing_times_;
};
```

#### 6.1.2 Health Check Interface
```cpp
class HealthChecker {
public:
    enum class HealthStatus {
        HEALTHY,
        DEGRADED,
        UNHEALTHY,
        UNKNOWN
    };
    
    struct ComponentHealth {
        std::string component_name;
        HealthStatus status;
        std::string status_message;
        std::chrono::time_point<std::chrono::steady_clock> last_check;
        std::map<std::string, std::string> details;
    };
    
    void registerComponent(const std::string& name, 
                          std::function<ComponentHealth()> health_check);
    ComponentHealth checkComponent(const std::string& name);
    std::vector<ComponentHealth> checkAllComponents();
    HealthStatus getOverallHealth();
    
private:
    std::unordered_map<std::string, std::function<ComponentHealth()>> health_checks_;
    std::shared_mutex health_checks_mutex_;
};
```

---

## 7. ERROR HANDLING INTERFACES

### 7.1 Error Reporting

#### 7.1.1 Error Types
```cpp
namespace errors {
    class SystemError : public std::exception {
    public:
        enum class ErrorCode {
            SENSOR_TIMEOUT,
            PROCESSING_OVERLOAD,
            MEMORY_EXHAUSTED,
            CONFIGURATION_ERROR,
            NETWORK_FAILURE,
            HARDWARE_FAILURE
        };
        
        SystemError(ErrorCode code, const std::string& message);
        ErrorCode getErrorCode() const { return error_code_; }
        const char* what() const noexcept override;
        
    private:
        ErrorCode error_code_;
        std::string message_;
    };
    
    class SafetyError : public SystemError {
    public:
        enum class SafetyLevel {
            WARNING,
            CRITICAL,
            CATASTROPHIC
        };
        
        SafetyError(ErrorCode code, SafetyLevel level, const std::string& message);
        SafetyLevel getSafetyLevel() const { return safety_level_; }
        
    private:
        SafetyLevel safety_level_;
    };
}
```

#### 7.1.2 Error Handler Interface
```cpp
class ErrorHandler {
public:
    using ErrorCallback = std::function<void(const std::exception&)>;
    
    void registerErrorHandler(const std::string& component, ErrorCallback callback);
    void reportError(const std::string& component, const std::exception& error);
    void reportSafetyError(const errors::SafetyError& error);
    
    struct ErrorStatistics {
        uint32_t total_errors;
        uint32_t errors_last_hour;
        std::map<std::string, uint32_t> errors_by_component;
        std::map<errors::SystemError::ErrorCode, uint32_t> errors_by_type;
    };
    
    ErrorStatistics getErrorStatistics() const;
    
private:
    std::unordered_map<std::string, ErrorCallback> error_handlers_;
    std::vector<std::pair<std::chrono::time_point<std::chrono::steady_clock>, 
                         std::string>> error_history_;
    std::shared_mutex error_handlers_mutex_;
};
```

---

**Document Control:**
- **Author:** Interface Design Team
- **Reviewed By:** System Architect, Integration Team
- **Approved By:** Technical Lead
- **Next Review Date:** [Monthly review during development]

*This interface specification ensures consistent communication protocols and data formats across all system components, enabling reliable integration and testing.*