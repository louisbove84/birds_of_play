# Birds of Play

A comprehensive system for bird tracking and analysis, combining motion detection, machine learning, and web interface.

## Project Components

### Motion Detection (C++)
- Real-time motion tracking using OpenCV
- Support for multiple cameras
- Initial detection of moving objects

### Machine Learning (Python)
- Bird species classification
- Behavior analysis
- Pattern recognition

### Frontend (Node.js)
- Web interface for real-time monitoring
- Data visualization
- User controls and settings

## Getting Started

Each component has its own setup instructions in its respective directory:

- [Motion Detection Setup](motion_detection/README.md)
- ML Setup (Coming Soon)
- Frontend Setup (Coming Soon)

## Development Status

- [x] Motion Detection (Initial Implementation)
- [ ] Machine Learning Integration
- [ ] Frontend Development

## CI/CD Pipeline Setup

### Required GitHub Secrets

The following secrets need to be configured in your GitHub repository settings (Settings > Secrets and variables > Actions):

#### DockerHub Secrets
- `DOCKERHUB_USERNAME`: Your DockerHub username
- `DOCKERHUB_TOKEN`: Your DockerHub access token (create at DockerHub > Account Settings > Security)

#### Ubuntu Deployment Secrets
These are required for automatic deployment to your Ubuntu laptop:
- `UBUNTU_HOST`: Your laptop's IP address or hostname
- `UBUNTU_USERNAME`: Your Ubuntu username
- `UBUNTU_SSH_KEY`: Private SSH key for GitHub Actions to access your Ubuntu laptop

### Setting up Ubuntu Deployment

1. **Install Docker on Ubuntu:**
   ```bash
   sudo apt-get update
   sudo apt-get install docker.io
   sudo usermod -aG docker $USER
   ```

2. **Set up SSH access:**
   ```bash
   # Generate SSH key
   ssh-keygen -t ed25519 -C "github-actions"
   
   # Add public key to authorized_keys
   cat ~/.ssh/id_ed25519.pub >> ~/.ssh/authorized_keys
   
   # Copy private key content for UBUNTU_SSH_KEY secret
   cat ~/.ssh/id_ed25519
   ```

3. **Verify Docker access:**
   ```bash
   # Test Docker installation
   docker run hello-world
   ```

## Contributing

This project is under active development. Please check the individual component directories for specific contribution guidelines.

## License

[Add your chosen license here] 