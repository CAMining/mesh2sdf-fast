
import mesh2sdf_fast as mesh2sdf
import sys

def test_manual_polygon():
    print("Testing manual polygon addition (Quad)...")
    mesh = mesh2sdf.Mesh()
    
    # Add 4 vertices for a quad (square)
    mesh.vertices = [
        mesh2sdf.Vec3(0, 0, 0),
        mesh2sdf.Vec3(1, 0, 0),
        mesh2sdf.Vec3(1, 1, 0),
        mesh2sdf.Vec3(0, 1, 0)
    ]
    
    # Add a single polygon with 4 indices [0, 1, 2, 3]
    # This should be automatically triangulated into 2 triangles: (0,1,2) and (0,2,3)
    mesh.add_polygon([0, 1, 2, 3])
    
    print(f"Vertex count: {mesh.vertex_count()} (Expected 4)")
    print(f"Triangle count: {mesh.triangle_count()} (Expected 2)")
    
    if mesh.triangle_count() == 2:
        print("SUCCESS: Quad was correctly triangulated into 2 triangles.")
    else:
        print("FAILURE: Incorrect triangle count.")
        sys.exit(1)

if __name__ == "__main__":
    try:
        test_manual_polygon()
        print("\nAll polygon support tests passed!")
    except Exception as e:
        print(f"\nTest failed with error: {e}")
        sys.exit(1)
