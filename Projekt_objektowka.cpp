#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"

#if defined(PLATFORM_DESKTOP)
#define GLSL_VERSION            330
#else   // PLATFORM_ANDROID, PLATFORM_WEB
#define GLSL_VERSION            120
#endif

#define SHADOWMAP_RESOLUTION 1024

RenderTexture2D LoadShadowmapRenderTexture(int width, int height);
void UnloadShadowmapRenderTexture(RenderTexture2D target);

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
        DrawModelEx(model1, position, { 0.0f, 1.0f, 0.0f }, 0.0f, { 0.1f, 0.1f, 0.1f }, DARKBLUE);
    }

};

class modele : public main_model { //polimorfizm ahh

public:
    
    //BoundingBox bounds;

    modele(const char* modelPath, Vector3 pos = { 0.0f, 0.0f, 0.0f })
        : main_model(modelPath, pos)
    {
        //bounds = GetMeshBoundingBox(model1.meshes[0]);
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
        DrawModelEx(model1, position, { 0.0f, 1.0f, 0.0f }, 0.0f, { 0.1f, 0.1f, 0.1f }, RED);
        //DrawBoundingBox(GetTransformedBoundingBox(), GREEN);
    }
    void Update() {
        if (IsKeyDown(KEY_W)) {
            position.y += stepSize;
        }
        if (IsKeyDown(KEY_S)) {
            position.y -= stepSize;
        }

    }
    /*
    BoundingBox GetTransformedBoundingBox() const {
        // Skalujemy i przesuwamy box zgodnie z modelem
        BoundingBox transformed = bounds;
        float scale = 0.1f;
        transformed.min = Vector3Add(Vector3Scale(transformed.min, scale), position);
        transformed.max = Vector3Add(Vector3Scale(transformed.max, scale), position);
        return transformed;
    }*/
private:
    int moveDirection = 1;
    int moveSteps = 0;
    const int maxSteps = 20;
    float stepSize = 0.5f;

};
/*
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
}*/

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 800;
    const int screenHeight = 450;

    SetConfigFlags(FLAG_MSAA_4X_HINT);
    // Shadows are a HUGE topic, and this example shows an extremely simple implementation of the shadowmapping algorithm,
    // which is the industry standard for shadows. This algorithm can be extended in a ridiculous number of ways to improve
    // realism and also adapt it for different scenes. This is pretty much the simplest possible implementation.
    InitWindow(screenWidth, screenHeight, "Projekt obiektowka");

    Camera3D cam = { 0 };
    cam.position = { 100.0f, 100.0f, 100.0f };
    cam.target = { 0.0f, 10.0f, 0.0f };
    cam.projection = CAMERA_PERSPECTIVE;
    cam.up = { 0.0f, 1.0f, 0.0f };
    cam.fovy = 45.0f;

    Shader shadowShader = LoadShader(TextFormat("shadery/shader%i.vs", GLSL_VERSION),
        TextFormat("shadery/shader%i.fs", GLSL_VERSION));
    shadowShader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(shadowShader, "viewPos");
    Vector3 lightDir = Vector3Normalize({ 0.35f, -1.0f, -0.35f });
    Color lightColor = WHITE;
    Vector4 lightColorNormalized = ColorNormalize(lightColor);
    int lightDirLoc = GetShaderLocation(shadowShader, "lightDir");
    int lightColLoc = GetShaderLocation(shadowShader, "lightColor");
    SetShaderValue(shadowShader, lightDirLoc, &lightDir, SHADER_UNIFORM_VEC3);
    SetShaderValue(shadowShader, lightColLoc, &lightColorNormalized, SHADER_UNIFORM_VEC4);
    int ambientLoc = GetShaderLocation(shadowShader, "ambient");
    float ambient[4] = { 0.1f, 0.1f, 0.1f, 1.0f };
    SetShaderValue(shadowShader, ambientLoc, ambient, SHADER_UNIFORM_VEC4);
    int lightVPLoc = GetShaderLocation(shadowShader, "lightVP");
    int shadowMapLoc = GetShaderLocation(shadowShader, "shadowMap");
    int shadowMapResolution = SHADOWMAP_RESOLUTION;
    SetShaderValue(shadowShader, GetShaderLocation(shadowShader, "shadowMapResolution"), &shadowMapResolution, SHADER_UNIFORM_INT);

    main_model drukarka("modele/main.obj", { 0.0f, 0.0f, 0.0f }); // Load model
    modele table("modele/Y.obj", { 0.0f, 5.0f, 0.0f });
    modele nozzle("modele/X.obj", { 0.0f, 31.5f, 0.0f });
    modele rail("modele/Z.obj", { 0.0f, 30.0f, 0.0f });

    for (int i = 0; i < drukarka.model1.materialCount; i++)
    {
        drukarka.model1.materials[i].shader = shadowShader;
    }
    
    bool selected = false;

    RenderTexture2D shadowMap = LoadShadowmapRenderTexture(SHADOWMAP_RESOLUTION, SHADOWMAP_RESOLUTION);
    // For the shadowmapping algorithm, we will be rendering everything from the light's point of view
    Camera3D lightCam = { 0 };
    lightCam.position = Vector3Scale(lightDir, -15.0f);
    lightCam.target = Vector3Zero();
    // Use an orthographic projection for directional lights
    lightCam.projection = CAMERA_ORTHOGRAPHIC;
    lightCam.up = { 0.0f, 1.0f, 0.0f };
    lightCam.fovy = 20.0f;

    DisableCursor();
    SetTargetFPS(60);

    // Main game loop
    while (!WindowShouldClose())    // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        float dt = GetFrameTime();

        Vector3 cameraPos = cam.position;
        SetShaderValue(shadowShader, shadowShader.locs[SHADER_LOC_VECTOR_VIEW], &cameraPos, SHADER_UNIFORM_VEC3);
        UpdateCamera(&cam, CAMERA_FREE);


        const float cameraSpeed = 0.05f;
        if (IsKeyDown(KEY_LEFT))
        {
            if (lightDir.x < 0.6f)
                lightDir.x += cameraSpeed * 60.0f * dt;
        }
        if (IsKeyDown(KEY_RIGHT))
        {
            if (lightDir.x > -0.6f)
                lightDir.x -= cameraSpeed * 60.0f * dt;
        }
        if (IsKeyDown(KEY_UP))
        {
            if (lightDir.z < 0.6f)
                lightDir.z += cameraSpeed * 60.0f * dt;
        }
        if (IsKeyDown(KEY_DOWN))
        {
            if (lightDir.z > -0.6f)
                lightDir.z -= cameraSpeed * 60.0f * dt;
        }
        lightDir = Vector3Normalize(lightDir);
        lightCam.position = Vector3Scale(lightDir, -15.0f);
        SetShaderValue(shadowShader, lightDirLoc, &lightDir, SHADER_UNIFORM_VEC3);

        // Draw
        //----------------------------------------------------------------------------------
        BeginDrawing();

        // First, render all objects into the shadowmap
        // The idea is, we record all the objects' depths (as rendered from the light source's point of view) in a buffer
        // Anything that is "visible" to the light is in light, anything that isn't is in shadow
        // We can later use the depth buffer when rendering everything from the player's point of view
        // to determine whether a given point is "visible" to the light

        // Record the light matrices for future use!
        Matrix lightView;
        Matrix lightProj;
        BeginTextureMode(shadowMap);
        ClearBackground(WHITE);
        BeginMode3D(lightCam);
        lightView = rlGetMatrixModelview();
        lightProj = rlGetMatrixProjection();
        DrawGrid(20, 10.0f);
        drukarka.Draw();
        table.Draw();
        nozzle.Draw();
        rail.Draw();
        EndMode3D();
        EndTextureMode();
        Matrix lightViewProj = MatrixMultiply(lightView, lightProj);

        ClearBackground(RAYWHITE);

        SetShaderValueMatrix(shadowShader, lightVPLoc, lightViewProj);

        rlEnableShader(shadowShader.id);
        int slot = 10; // Can be anything 0 to 15, but 0 will probably be taken up
        rlActiveTextureSlot(10);
        rlEnableTexture(shadowMap.depth.id);
        rlSetUniform(shadowMapLoc, &slot, SHADER_UNIFORM_INT, 1);

        BeginMode3D(cam);

        // Draw the same exact things as we drew in the shadowmap!
        DrawGrid(20, 10.0f);
        drukarka.Draw();
        table.Draw();
        nozzle.Draw();
        rail.Draw();

        EndMode3D();
        if (selected) DrawText("MODEL SELECTED", GetScreenWidth() - 110, 10, 10, GREEN);

        //SprawdzKolizje(table, rail);
        DrawFPS(10, 10);
        EndDrawing();

        if (IsKeyPressed(KEY_F))
        {
            TakeScreenshot("shaders_shadowmap.png");
        }
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------

    UnloadShader(shadowShader);
    UnloadShadowmapRenderTexture(shadowMap);

    CloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}

RenderTexture2D LoadShadowmapRenderTexture(int width, int height)
{
    RenderTexture2D target = { 0 };

    target.id = rlLoadFramebuffer(); // Load an empty framebuffer
    target.texture.width = width;
    target.texture.height = height;

    if (target.id > 0)
    {
        rlEnableFramebuffer(target.id);

        // Create depth texture
        // We don't need a color texture for the shadowmap
        target.depth.id = rlLoadTextureDepth(width, height, false);
        target.depth.width = width;
        target.depth.height = height;
        target.depth.format = 19;       //DEPTH_COMPONENT_24BIT?
        target.depth.mipmaps = 1;

        // Attach depth texture to FBO
        rlFramebufferAttach(target.id, target.depth.id, RL_ATTACHMENT_DEPTH, RL_ATTACHMENT_TEXTURE2D, 0);

        // Check if fbo is complete with attachments (valid)
        if (rlFramebufferComplete(target.id)) TRACELOG(LOG_INFO, "FBO: [ID %i] Framebuffer object created successfully", target.id);

        rlDisableFramebuffer();
    }
    else TRACELOG(LOG_WARNING, "FBO: Framebuffer object can not be created");

    return target;
}

// Unload shadowmap render texture from GPU memory (VRAM)
void UnloadShadowmapRenderTexture(RenderTexture2D target)
{
    if (target.id > 0)
    {
        // NOTE: Depth texture/renderbuffer is automatically
        // queried and deleted before deleting framebuffer
        rlUnloadFramebuffer(target.id);
    }
}