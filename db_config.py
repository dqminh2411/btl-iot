import pymysql
from pymysql.cursors import DictCursor
from dotenv import load_dotenv
import os

env_path = '.env.dev'
load_dotenv(dotenv_path=env_path)
db_password = os.getenv("DB_PASSWORD")
db_host = os.getenv("DB_HOST")
db_username = os.getenv("DB_USERNAME")
db_name = os.getenv("DB_NAME")

# Database configuration
DB_CONFIG = {
    'host': db_host,
    'user': db_username,
    'password': db_password,  
    'database': db_name,  
    'charset': 'utf8mb4',
    'cursorclass': DictCursor
}

def get_db_connection():
    """
    Create and return a database connection
    
    Returns:
        pymysql.connections.Connection: Database connection object
    """
    try:
        connection = pymysql.connect(**DB_CONFIG)
        return connection
    except pymysql.Error as e:
        print(f"Error connecting to MySQL: {e}")
        return None

def init_db():
    """
    Initialize database and create tables if they don't exist
    """
    connection = get_db_connection()
    if not connection:
        print("Failed to connect to database")
        return False
    
    try:
        with connection.cursor() as cursor:
            # Create folders table
            cursor.execute("""
                CREATE TABLE IF NOT EXISTS folders (
                    id INT AUTO_INCREMENT PRIMARY KEY,
                    name VARCHAR(255) NOT NULL,
                    create_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                    INDEX idx_create_time (create_time)
                )
            """)
            
            # Create images table
            cursor.execute("""
                CREATE TABLE IF NOT EXISTS images (
                    id INT AUTO_INCREMENT PRIMARY KEY,
                    folder_id INT,
                    filename VARCHAR(255) NOT NULL,
                    filepath VARCHAR(500) NOT NULL,
                    file_size INT,
                    upload_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                    device_id VARCHAR(100) DEFAULT 'ESP32-CAM',
                    FOREIGN KEY (folder_id) REFERENCES folders(id) ON DELETE CASCADE,
                    INDEX idx_upload_time (upload_time),
                    INDEX idx_device_id (device_id),
                    INDEX idx_folder_id (folder_id)
                )
            """)
            
        connection.commit()
        print("Database initialized successfully")
        return True
        
    except pymysql.Error as e:
        print(f"Error initializing database: {e}")
        return False
        
    finally:
        connection.close()

def create_folder(name):
    """
    Create a new folder
    
    Args:
        name (str): Name of the folder
    
    Returns:
        int: ID of the created folder, or None if failed
    """
    connection = get_db_connection()
    if not connection:
        return None
    
    try:
        with connection.cursor() as cursor:
            sql = "INSERT INTO folders (name) VALUES (%s)"
            cursor.execute(sql, (name,))
        
        connection.commit()
        return cursor.lastrowid
        
    except pymysql.Error as e:
        print(f"Error creating folder: {e}")
        return None
        
    finally:
        connection.close()

def get_all_folders():
    """
    Retrieve all folders
    
    Returns:
        list: List of folder records
    """
    connection = get_db_connection()
    if not connection:
        return []
    
    try:
        with connection.cursor() as cursor:
            sql = """
                SELECT f.*, COUNT(i.id) as image_count
                FROM folders f
                LEFT JOIN images i ON f.id = i.folder_id
                GROUP BY f.id
                ORDER BY f.create_time DESC
            """
            cursor.execute(sql)
            return cursor.fetchall()
            
    except pymysql.Error as e:
        print(f"Error retrieving folders: {e}")
        return []
        
    finally:
        connection.close()

def get_folder_by_id(folder_id):
    """
    Retrieve a specific folder by ID
    
    Args:
        folder_id (int): ID of the folder
    
    Returns:
        dict: Folder record or None if not found
    """
    connection = get_db_connection()
    if not connection:
        return None
    
    try:
        with connection.cursor() as cursor:
            sql = "SELECT * FROM folders WHERE id = %s"
            cursor.execute(sql, (folder_id,))
            return cursor.fetchone()
            
    except pymysql.Error as e:
        print(f"Error retrieving folder: {e}")
        return None
        
    finally:
        connection.close()

def save_image_record(filename, filepath, file_size, folder_id=None, device_id='ESP32-CAM'):
    """
    Save image metadata to database
    
    Args:
        filename (str): Name of the image file
        filepath (str): Path to the image file
        file_size (int): Size of the file in bytes
        folder_id (int): ID of the folder (optional)
        device_id (str): ID of the device that uploaded the image
    
    Returns:
        int: ID of the inserted record, or None if failed
    """
    connection = get_db_connection()
    if not connection:
        return None
    
    try:
        with connection.cursor() as cursor:
            sql = """
                INSERT INTO images (folder_id, filename, filepath, file_size, device_id)
                VALUES (%s, %s, %s, %s, %s)
            """
            cursor.execute(sql, (folder_id, filename, filepath, file_size, device_id))
        
        connection.commit()
        return cursor.lastrowid
        
    except pymysql.Error as e:
        print(f"Error saving image record: {e}")
        return None
        
    finally:
        connection.close()

def get_all_images(limit=100, folder_id=None):
    """
    Retrieve all image records from database
    
    Args:
        limit (int): Maximum number of records to return
        folder_id (int): Filter by folder ID (optional)
    
    Returns:
        list: List of image records as dictionaries
    """
    connection = get_db_connection()
    if not connection:
        return []
    
    try:
        with connection.cursor() as cursor:
            if folder_id:
                sql = """
                    SELECT i.*, f.name as folder_name
                    FROM images i
                    LEFT JOIN folders f ON i.folder_id = f.id
                    WHERE i.folder_id = %s
                    ORDER BY i.upload_time DESC
                    LIMIT %s
                """
                cursor.execute(sql, (folder_id, limit))
            else:
                sql = """
                    SELECT i.*, f.name as folder_name
                    FROM images i
                    LEFT JOIN folders f ON i.folder_id = f.id
                    ORDER BY i.upload_time DESC
                    LIMIT %s
                """
                cursor.execute(sql, (limit,))
            return cursor.fetchall()
            
    except pymysql.Error as e:
        print(f"Error retrieving images: {e}")
        return []
        
    finally:
        connection.close()

def get_all_images_unassigned(limit=100):
    connection = get_db_connection()
    if not connection:
        return []
    
    try:
        with connection.cursor() as cursor:
            sql = """
                SELECT i.*, f.name as folder_name
                FROM images i
                LEFT JOIN folders f ON i.folder_id = f.id
                WHERE i.folder_id IS NULL
                ORDER BY i.upload_time DESC
                LIMIT %s
            """
            cursor.execute(sql, (limit))
            return cursor.fetchall()
            
    except pymysql.Error as e:
        print(f"Error retrieving images: {e}")
        return []
        
    finally:
        connection.close()

def get_image_by_id(image_id):
    """
    Retrieve a specific image record by ID
    
    Args:
        image_id (int): ID of the image record
    
    Returns:
        dict: Image record or None if not found
    """
    connection = get_db_connection()
    if not connection:
        return None
    
    try:
        with connection.cursor() as cursor:
            sql = "SELECT * FROM images WHERE id = %s"
            cursor.execute(sql, (image_id,))
            return cursor.fetchone()
            
    except pymysql.Error as e:
        print(f"Error retrieving image: {e}")
        return None
        
    finally:
        connection.close()

def delete_image_record(image_id):
    """
    Delete an image record from database
    
    Args:
        image_id (int): ID of the image record to delete
    
    Returns:
        bool: True if successful, False otherwise
    """
    connection = get_db_connection()
    if not connection:
        return False
    
    try:
        with connection.cursor() as cursor:
            sql = "DELETE FROM images WHERE id = %s"
            cursor.execute(sql, (image_id,))
        
        connection.commit()
        return True
        
    except pymysql.Error as e:
        print(f"Error deleting image record: {e}")
        return False
        
    finally:
        connection.close()

def add_images_to_folder(image_ids, folder_id):
    """
    Add multiple images to a folder
    
    Args:
        image_ids (list): List of image IDs to add to folder
        folder_id (int): ID of the folder
    
    Returns:
        dict: Result with success status and updated count
    """
    connection = get_db_connection()
    if not connection:
        return {'success': False, 'error': 'Database connection failed', 'updated': 0}
    
    try:
        with connection.cursor() as cursor:
            # Verify folder exists
            cursor.execute("SELECT id FROM folders WHERE id = %s", (folder_id,))
            if not cursor.fetchone():
                return {'success': False, 'error': 'Folder not found', 'updated': 0}
            
            # Update images
            if isinstance(image_ids, list):
                placeholders = ','.join(['%s'] * len(image_ids))
                sql = f"UPDATE images SET folder_id = %s WHERE id IN ({placeholders})"
                cursor.execute(sql, [folder_id] + image_ids)
            else:
                sql = "UPDATE images SET folder_id = %s WHERE id = %s"
                cursor.execute(sql, (folder_id, image_ids))
            
            updated_count = cursor.rowcount
        
        connection.commit()
        return {'success': True, 'updated': updated_count}
        
    except pymysql.Error as e:
        print(f"Error adding images to folder: {e}")
        return {'success': False, 'error': str(e), 'updated': 0}
        
    finally:
        connection.close()

if __name__ == '__main__':
    print("Testing database connection...")
    if init_db():
        print("✓ Database connection successful!")
        print("✓ Tables created/verified")
    else:
        print("✗ Database connection failed")
