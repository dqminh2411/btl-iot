from flask import Flask, request, jsonify
import os
from datetime import datetime
from db_config import init_db, save_image_record, get_all_images, create_folder, get_all_folders, get_folder_by_id, add_images_to_folder

app = Flask(__name__)

# Configuration
UPLOAD_FOLDER = 'uploads'
ALLOWED_EXTENSIONS = {'jpg', 'jpeg', 'png', 'gif', 'bmp'}

# Create uploads folder if it doesn't exist
os.makedirs(UPLOAD_FOLDER, exist_ok=True)

# Initialize database
init_db()

# Add request logging
@app.before_request
def log_request_info():
    print(f"\n{'='*50}")
    print(f"Incoming Request: {request.method} {request.path}")
    print(f"From: {request.remote_addr}")
    print(f"Content-Type: {request.content_type}")
    print(f"Content-Length: {request.content_length}")
    print(f"Headers: {dict(request.headers)}")
    print(f"{'='*50}\n")

def allowed_file(filename):
    return '.' in filename and filename.rsplit('.', 1)[1].lower() in ALLOWED_EXTENSIONS

@app.route('/')
def index():
    return jsonify({
        'status': 'running',
        'message': 'ESP32-CAM Image Upload API',
        'endpoints': {
            '/upload': 'POST - Upload image from ESP32-CAM',
            '/folders': 'GET - List all folders, POST - Create folder',
            '/folders/<id>': 'GET - Get folder details',
            '/folders/<id>/add_images': 'POST - Add images to folder',
            '/images': 'GET - List all images'
        }
    })

@app.route('/upload', methods=['POST'])
def upload_image():
    try:
        print(f"Request data length: {len(request.data)}")
        # Try to get raw binary data
        if request.data:
            # Generate filename with timestamp
            timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
            filename = f'{timestamp}.jpg'
            filepath = os.path.join(UPLOAD_FOLDER, filename)
            
            # Save the image
            with open(filepath, 'wb') as f:
                f.write(request.data)
            
            print(f"Image saved to: {filepath}")
            
            # Save to database (folder_id is None by default)
            file_size = len(request.data)
            image_id = save_image_record(filename, filepath, file_size)
            
            print(f"Image record saved with ID: {image_id}")
            
            return jsonify({
                'success': True,
                'message': 'Image uploaded successfully',
                'filename': filename,
                'size': file_size,
                'image_id': image_id
            }), 200
        else:
            print("No data in request!")
            return jsonify({
                'success': False,
                'error': 'No image data found'
            }), 400
        
    except Exception as e:
        print(f"ERROR in upload_image: {str(e)}")
        import traceback
        traceback.print_exc()
        return jsonify({
            'success': False,
            'error': str(e)
        }), 500

@app.route('/folders', methods=['GET', 'POST'])
def manage_folders():
    """List all folders or create a new folder"""
    if request.method == 'POST':
        try:
            data = request.get_json() or {}
            name = data.get('name')
            
            if not name:
                return jsonify({
                    'success': False,
                    'error': 'Folder name is required'
                }), 400
            
            folder_id = create_folder(name)
            
            if folder_id:
                return jsonify({
                    'success': True,
                    'message': 'Folder created successfully',
                    'folder_id': folder_id,
                    'name': name
                }), 201
            else:
                return jsonify({
                    'success': False,
                    'error': 'Failed to create folder'
                }), 500
                
        except Exception as e:
            return jsonify({
                'success': False,
                'error': str(e)
            }), 500
    else:  # GET
        try:
            folders = get_all_folders()
            return jsonify({
                'success': True,
                'count': len(folders),
                'folders': folders
            }), 200
        except Exception as e:
            return jsonify({
                'success': False,
                'error': str(e)
            }), 500

@app.route('/folders/<int:folder_id>', methods=['GET'])
def get_folder(folder_id):
    """Get folder details and its images"""
    try:
        folder = get_folder_by_id(folder_id)
        
        if not folder:
            return jsonify({
                'success': False,
                'error': 'Folder not found'
            }), 404
        
        # Get images in this folder
        images = get_all_images(folder_id=folder_id)
        
        return jsonify({
            'success': True,
            'folder': folder,
            'images': images,
            'image_count': len(images)
        }), 200
        
    except Exception as e:
        return jsonify({
            'success': False,
            'error': str(e)
        }), 500

@app.route('/folders/<int:folder_id>/add_images', methods=['POST'])
def add_images_to_folder_endpoint(folder_id):
    """Add images to a folder"""
    try:
        data = request.get_json() or {}
        image_ids = data.get('image_ids')
        
        if not image_ids:
            return jsonify({
                'success': False,
                'error': 'image_ids array is required'
            }), 400
        
        if not isinstance(image_ids, list):
            return jsonify({
                'success': False,
                'error': 'image_ids must be an array'
            }), 400
        
        # Add images to folder
        result = add_images_to_folder(image_ids, folder_id)
        
        if result['success']:
            return jsonify({
                'success': True,
                'message': f'{result["updated"]} image(s) added to folder',
                'updated_count': result['updated']
            }), 200
        else:
            return jsonify({
                'success': False,
                'error': result.get('error', 'Failed to add images to folder')
            }), 400
            
    except Exception as e:
        return jsonify({
            'success': False,
            'error': str(e)
        }), 500

@app.route('/images', methods=['GET'])
def list_images():
    """List all uploaded images from database"""
    try:
        limit = request.args.get('limit', 100, type=int)
        folder_id = request.args.get('folder_id', type=int)
        images = get_all_images(limit, folder_id)
        
        return jsonify({
            'success': True,
            'count': len(images),
            'images': images
        }), 200
    except Exception as e:
        return jsonify({
            'success': False,
            'error': str(e)
        }), 500

if __name__ == '__main__':
    
    app.run(host='0.0.0.0', port=5000, debug=False)
