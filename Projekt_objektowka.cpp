
#include "raylib.h"
#include "raymath.h" 

class main_model { //klasa dla glownego modelu
public:
	Model model1;
	Vector3 position;

	main_model(const char* modelPath, Vector3 pos = { 0.0f, 0.0f, 0.0f })
		: position(pos)
	{
		model1 = LoadModel(modelPath);
	}

	~main_model() {
		UnloadModel(model1);
	}

	virtual void Draw() {
		DrawModel(model1, position, 0.1f, DARKBLUE);
	}

};

class modele: public main_model{ //polimorfizm ahh

public:

    BoundingBox bounds;

    modele(const char* modelPath, Vector3 pos = { 0.0f, 0.0f, 0.0f })
        : main_model(modelPath, pos) 
    {
        bounds = GetMeshBoundingBox(model1.meshes[0]);
    }

    ~modele() {
        // Nie trzeba zwalniać model1, bo robi to destruktor klasy bazowej
    }


    void Draw() override {

        if (IsKeyDown(KEY_W)) {
            position.y += stepSize;
        }
        if (IsKeyDown(KEY_S)) {
            position.y -= stepSize;
        }
        DrawModel(model1, position, 0.1f, RED);
        DrawBoundingBox(GetTransformedBoundingBox(), GREEN);
    }
    void Update() {
        if (IsKeyDown(KEY_W)) {
            position.y += stepSize;
        }
        if (IsKeyDown(KEY_S)) {
            position.y -= stepSize;
        }
    
    }

    BoundingBox GetTransformedBoundingBox() const {
        // Skalujemy i przesuwamy box zgodnie z modelem
        BoundingBox transformed = bounds;
        float scale = 0.1f;
        transformed.min = Vector3Add(Vector3Scale(transformed.min, scale), position);
        transformed.max = Vector3Add(Vector3Scale(transformed.max, scale), position);
        return transformed;
    }
private:
        int moveDirection = 1;
        int moveSteps = 0;
        const int maxSteps = 20;
        float stepSize = 0.5f;

};

void SprawdzKolizje(modele& model1, modele& model2) {
	bool collision = false; // Flaga kolizji
    BoundingBox bounds1 = model1.bounds;
    BoundingBox bounds2 = model2.bounds;

    collision = CheckCollisionBoxes(model1.GetTransformedBoundingBox(), model2.GetTransformedBoundingBox()); // Ustaw flagę kolizji

    if (collision) {
        // Tutaj możesz dodać dodatkowe akcje, np. zmianę koloru, zatrzymanie ruchu itp.
		// Przykład: zmiana koloru modelu na czerwony
		DrawText("KOLIZJA!", 10, 10, 20, RED);
	}
	else {
		// Jeśli nie ma kolizji, przywróć domyślny kolor
		DrawText("BRAK KOLIZJI", 10, 10, 20, GREEN);
    }
}

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 800;
    const int screenHeight = 450;
    InitWindow(screenWidth, screenHeight, "raylib [models] example - models loading");

    // Define the camera to look into our 3d world
       Camera camera = { 0 };
       camera.position = { 100.0f, 100.0f, 100.0f }; // Camera position
       camera.target = { 0.0f, 10.0f, 0.0f };     // Camera looking at point
       camera.up = { 0.0f, 1.0f, 0.0f };          // Camera up vector (rotation towards target)
       camera.fovy = 55.0f;                       // Camera field-of-view Y
       camera.projection = CAMERA_PERSPECTIVE;    // Camera mode type

	main_model drukarka("main.obj", { 0.0f, 0.0f, 0.0f }); // Load model
    modele table("Y.obj", { 0.0f, 5.0f, 0.0f });
	modele nozzle("X.obj", { 0.0f, 31.5f, 0.0f });
	modele rail("Z.obj", { 0.0f, 30.0f, 0.0f }); // { 0.0f, 30.0f, 0.0f } });
    //Texture2D texture = LoadTexture("resources/models/obj/castle_diffuse.png"); // Load model texture 
    //model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;            // Set map diffuse texture

    Vector3 position = { 0.0f, 0.0f, 0.0f };                    // Set model position

    //BoundingBox bounds = GetMeshBoundingBox(model.meshes[0]);   // Set model bounds

    // NOTE: bounds are calculated from the original size of the model,
    // if model is scaled on drawing, bounds must be also scaled

    bool selected = false;          // Selected object flag

    DisableCursor();                // Limit cursor to relative movement inside the window

    SetTargetFPS(60);               // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!WindowShouldClose())    // Detect window close button or ESC key
    {

        // Update
        //----------------------------------------------------------------------------------
        UpdateCamera(&camera, CAMERA_FREE);

        // Load new models/textures on drag&drop
        if (IsFileDropped())
        {
            FilePathList droppedFiles = LoadDroppedFiles();

            if (droppedFiles.count == 1) // Only support one file dropped
            {
                if (IsFileExtension(droppedFiles.paths[0], ".obj") ||
                    IsFileExtension(droppedFiles.paths[0], ".gltf") ||
                    IsFileExtension(droppedFiles.paths[0], ".glb") ||
                    IsFileExtension(droppedFiles.paths[0], ".vox") ||
                    IsFileExtension(droppedFiles.paths[0], ".iqm") ||
                    IsFileExtension(droppedFiles.paths[0], ".m3d"))       // Model file formats supported
                {
                    //UnloadModel(model);                         // Unload previous model
                   // model = LoadModel(droppedFiles.paths[0]);   // Load new model
                    //model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture; // Set current map diffuse texture

                    //bounds = GetMeshBoundingBox(model.meshes[0]);

                    // TODO: Move camera position from target enough distance to visualize model properly
                }
                else if (IsFileExtension(droppedFiles.paths[0], ".png"))  // Texture file formats supported
                {
                    // Unload current model texture and load new one
                    //UnloadTexture(texture);
                    //texture = LoadTexture(droppedFiles.paths[0]);
                    //model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;
                }
            }

           // UnloadDroppedFiles(droppedFiles);    // Unload filepaths from memory
        }

        // Select model on mouse click
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            // Check collision between ray and box
           // if (GetRayCollisionBox(GetScreenToWorldRay(GetMousePosition(), camera), bounds).hit) selected = !selected;
           // else selected = false;
        }
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        BeginDrawing();

        ClearBackground(RAYWHITE);

        BeginMode3D(camera);

		drukarka.Draw();
        table.Draw();
		nozzle.Draw();
		rail.Draw();

        //rail.Update();
        //myTable.Update();
        DrawGrid(20, 10.0f);         // Draw a grid

        //if (selected) DrawBoundingBox(bounds, GREEN);   // Draw selection box

        EndMode3D();

        DrawText("Drag & drop model to load mesh/texture.", 10, GetScreenHeight() - 20, 10, DARKGRAY);
        if (selected) DrawText("MODEL SELECTED", GetScreenWidth() - 110, 10, 10, GREEN);

        SprawdzKolizje(table , rail);

        DrawFPS(10, 10);

        EndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    //UnloadTexture(texture);     // Unload texture
        // Unload model

    CloseWindow();              // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}