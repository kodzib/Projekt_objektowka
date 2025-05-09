#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <stdlib.h>
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"

#if defined(PLATFORM_DESKTOP)
#define GLSL_VERSION            330
#else   // PLATFORM_ANDROID, PLATFORM_WEB
#define GLSL_VERSION            120
#endif

#define SHADOWMAP_RESOLUTION 2048
#define MAX_FILEPATH_SIZE    2048

RenderTexture2D LoadShadowmapRenderTexture(int width, int height);
void UnloadShadowmapRenderTexture(RenderTexture2D target);
void GcodeAnalizer(std::string file_path, std::vector<Vector4>& points);

class main_model { //glowny model
public:
    Model model1;
    Vector3 position;

    main_model(const char* modelPath,Shader shader, Vector3 pos = { 0.0f, 0.0f, 0.0f }) : position(pos) {
        model1 = LoadModel(modelPath);
        for (int i = 0; i < model1.materialCount; i++) { //low key trzeba ogarnąć o co z tych chodzi, 
            model1.materials[i].shader = shader;
        }
    }

    ~main_model() {
        UnloadModel(model1);
    }

    virtual void Draw() {
        DrawModelEx(model1, position, { 0.0f, 1.0f, 0.0f }, 0.0f, { 0.1f, 0.1f, 0.1f }, DARKBLUE);
    }
};

class modele : public main_model { //polimorfizm 
public:
    modele(const char* modelPath, Shader shader, Vector3 pos = { 0.0f, 0.0f, 0.0f }) : main_model(modelPath,shader, pos) {
		//Nic nie potrzebne, bo używamy z klasy bazowej
    }

    ~modele() {
        //Nic nie potrzebne, bo używamy z klasy bazowej
    }

    void Draw() override {
        DrawModelEx(model1, position, { 0.0f, 1.0f, 0.0f }, 0.0f, { 0.1f, 0.1f, 0.1f }, RED);
    }
private:

    float stepSize = 0.5f;
};

class TargetPoint {
public:
    std::vector<Vector4> Target_positions;  
    float speed;
    int index = 0; 
    TargetPoint(std::vector<Vector4> pos, float s) : Target_positions(pos), speed(s) {}

    ~TargetPoint() {}

    void MoveToPoint(modele* x, modele* y, modele* z) {
        float epsilon = 0.01f; //kryterium jakosciowe ahh
        if (index < 0 || index >= Target_positions.size()) return;  

        Vector4 target = Target_positions[index]/10;
        TraceLog(LOG_INFO, TextFormat("--- Ruch do punktu %d ---", index));
        TraceLog(LOG_INFO, TextFormat("Cel: X=%.2f, Y=%.2f, Z=%.2f, F=%.2f", target.x, target.y, target.z, target.w));
        TraceLog(LOG_INFO, TextFormat("Pozycja dyszy (X): %.2f", x->position.x));
        TraceLog(LOG_INFO, TextFormat("Pozycja szyny (Y): %.2f", y->position.y));
        TraceLog(LOG_INFO, TextFormat("Pozycja stolu (Z): %.2f", z->position.z));
        // Ruch w osi X
        if (x->position.x < target.x && abs((x->position.x - target.x)) >epsilon) {
            x->position.x += speed;
        }
        else if (abs((x->position.x - target.x)) > epsilon){
            x->position.x -= speed;
        }

        // Ruch w osi Y
        if (y->position.y < target.y && abs((y->position.y - target.y)) >epsilon) {
            y->position.y += speed;
            x->position.y += speed;
        }
        else if (abs((y->position.y - target.y)) > epsilon) {
            y->position.y -= speed;
            x->position.y -= speed;
        }

        // Ruch w osi Z
        if (z->position.z < target.z  && abs((z->position.z - target.z )) >epsilon) {
            z->position.z += speed;
        }
        else if(abs((z->position.z - target.z)) > epsilon) {
            z->position.z -= speed;
        }

		if (abs((x->position.x - target.x)) <= epsilon && abs((y->position.y - target.y)) <= epsilon && abs((z->position.z - target.z)) <= epsilon){
            index++;
		}
    }
};

int main(void) {
    TargetPoint cel({}, 0.009f);
    const int screenWidth = 1600;
    const int screenHeight = 900;
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(screenWidth, screenHeight, "Projekt obiektowka");

    Camera3D cam = { 0 };
    cam.position = { 0.0f, 60.0f, 100.0f };
    cam.target = { 0.0f, 25.0f, 0.0f };
    cam.projection = CAMERA_PERSPECTIVE;
    cam.up = { 0.0f, 1.0f, 0.0f };
    cam.fovy = 45.0f;

    Shader shadowShader = LoadShader(TextFormat("shadery/shader%i.vs", GLSL_VERSION), TextFormat("shadery/shader%i.fs", GLSL_VERSION));
    shadowShader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(shadowShader, "viewPos");
    Vector3 lightDir = Vector3Normalize({ 0.1f, -1.0f, -0.35f });
    Color lightColor = WHITE;
    Vector4 lightColorNormalized = ColorNormalize(lightColor);
    int lightDirLoc = GetShaderLocation(shadowShader, "lightDir");
    int lightColLoc = GetShaderLocation(shadowShader, "lightColor");
    SetShaderValue(shadowShader, lightDirLoc, &lightDir, SHADER_UNIFORM_VEC3);
    SetShaderValue(shadowShader, lightColLoc, &lightColorNormalized, SHADER_UNIFORM_VEC4);
    int ambientLoc = GetShaderLocation(shadowShader, "ambient");
    float ambient[4] = { 0.3f, 0.3f, 0.3f, 0.7f };
    SetShaderValue(shadowShader, ambientLoc, ambient, SHADER_UNIFORM_VEC4);
    int lightVPLoc = GetShaderLocation(shadowShader, "lightVP");
    int shadowMapLoc = GetShaderLocation(shadowShader, "shadowMap");
    int shadowMapResolution = SHADOWMAP_RESOLUTION;
    SetShaderValue(shadowShader, GetShaderLocation(shadowShader, "shadowMapResolution"), &shadowMapResolution, SHADER_UNIFORM_INT);

    main_model drukarka("modele/main.obj", shadowShader ,{ 0.0f, 0.0f, 0.0f });
    //modele table("modele/Y.obj", shadowShader, { 0.0f, 5.4f, 0.0f });
    //modele nozzle("modele/X.obj", shadowShader, { 0.0f, 31.4f, 0.0f });
    //modele rail("modele/Z.obj", shadowShader, { 0.0f, 30.0f, 0.0f });
    modele table("modele/Y.obj", shadowShader, { 0.0f, 0.0f, 0.0f });
    modele nozzle("modele/X.obj", shadowShader, { 0.0f, 0.0f, 0.0f });
    modele rail("modele/Z.obj", shadowShader, { 0.0f, 0.0f, 0.0f });
    bool selected = false;

    RenderTexture2D shadowMap = LoadShadowmapRenderTexture(SHADOWMAP_RESOLUTION, SHADOWMAP_RESOLUTION);
    Camera3D lightCam = { 0 };
    lightCam.position = Vector3Scale(lightDir, -15.0f);
    lightCam.target = Vector3Zero();
    lightCam.projection = CAMERA_ORTHOGRAPHIC;
    lightCam.up = { 0.0f, 1.0f, 0.0f };
    lightCam.fovy = 20.0f;

    DisableCursor();
    SetTargetFPS(60);

    // Main game loop
    while (!WindowShouldClose()) {

        float dt = GetFrameTime();

        Vector3 cameraPos = cam.position;
        SetShaderValue(shadowShader, shadowShader.locs[SHADER_LOC_VECTOR_VIEW], &cameraPos, SHADER_UNIFORM_VEC3);
        UpdateCamera(&cam, CAMERA_FREE);

        const float cameraSpeed = 0.05f;
        if (IsKeyDown(KEY_LEFT)) {
            if (lightDir.x < 0.6f)
                lightDir.x += cameraSpeed * 60.0f * dt;
        }
        if (IsKeyDown(KEY_RIGHT)) {
            if (lightDir.x > -0.6f)
                lightDir.x -= cameraSpeed * 60.0f * dt;
        }
        if (IsKeyDown(KEY_UP)) {
            if (lightDir.z < 0.6f)
                lightDir.z += cameraSpeed * 60.0f * dt;
        }
        if (IsKeyDown(KEY_DOWN)) {
            if (lightDir.z > -0.6f)
                lightDir.z -= cameraSpeed * 60.0f * dt;
        }
        lightDir = Vector3Normalize(lightDir);
        lightCam.position = Vector3Scale(lightDir, -15.0f);
        SetShaderValue(shadowShader, lightDirLoc, &lightDir, SHADER_UNIFORM_VEC3);

        // Draw
        BeginDrawing();

        Matrix lightView;
        Matrix lightProj;
        BeginTextureMode(shadowMap);
        ClearBackground(WHITE);
        BeginMode3D(lightCam);
        lightView = rlGetMatrixModelview();
        lightProj = rlGetMatrixProjection();
        // RYSOWANIE

        DrawGrid(20, 10.0f); //

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
        // RYSOWANIE
        DrawGrid(20, 10.0f); //
        drukarka.Draw(); //
        table.Draw(); //
        nozzle.Draw(); //
        rail.Draw(); //

        if (IsFileDropped()) {
            char filePath[MAX_FILEPATH_SIZE] = { 0 };
            FilePathList droppedFiles = LoadDroppedFiles();
            TextCopy(filePath, droppedFiles.paths[0]);
            std::cout << filePath << std::endl;
            GcodeAnalizer(std::string(filePath), cel.Target_positions);
            UnloadDroppedFiles(droppedFiles);
        }

        cel.MoveToPoint(&nozzle, &rail, &table);

        EndMode3D();
        if (selected) DrawText("MODEL SELECTED", GetScreenWidth() - 110, 10, 10, GREEN);

        DrawFPS(10, 10);
        EndDrawing();

        if (IsKeyPressed(KEY_T)) {
            TakeScreenshot("shaders_shadowmap.png");
        }
    }

    // De-Initialization

    UnloadShader(shadowShader);
    UnloadShadowmapRenderTexture(shadowMap);

    CloseWindow();
    
    return 0;
}

RenderTexture2D LoadShadowmapRenderTexture(int width, int height) {
    RenderTexture2D target = { 0 };

    target.id = rlLoadFramebuffer(); // Load an empty framebuffer
    target.texture.width = width;
    target.texture.height = height;

    if (target.id > 0) {
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
void UnloadShadowmapRenderTexture(RenderTexture2D target) {
    if (target.id > 0) {
        rlUnloadFramebuffer(target.id);
    }
}

void GcodeAnalizer(std::string file_path, std::vector<Vector4>& points) {  
  std::fstream gcode_file;  
  gcode_file.open(file_path, std::ios::in);  
  if (gcode_file.is_open()) {  
      std::string line;  
      while (std::getline(gcode_file, line)) {  
          if (line.rfind("G1", 0) == 0) {  
              float X = 0.0f;  
              float Y = 0.0f;  
              float Z = 0.0f;
              float F = 0.0f;
              int x_pos = line.find("X");  
              int y_pos = line.find("Y");  
              int z_pos = line.find("Z");
			  int f_pos = line.find("F");
              int space_pos = 0;  
              if (x_pos > 0) {  
                  space_pos = line.find(" ", x_pos);  
                  X = std::stof(line.substr(x_pos + 1, space_pos - x_pos - 1));  
              }  
              space_pos = 0;  
              if (y_pos > 0) {  
                  space_pos = line.find(" ", y_pos);  
                  Y = std::stof(line.substr(y_pos + 1, space_pos - y_pos - 1));  
              }  
              space_pos = 0;  
              if (z_pos > 0) {  
                  space_pos = line.find(" ", z_pos);  
                  Z = std::stof(line.substr(z_pos + 1, space_pos - z_pos - 1));  
              }
              space_pos = 0;
              if (f_pos > 0) {
                  space_pos = line.find(" ", f_pos);
                  F = std::stof(line.substr(f_pos + 1, space_pos - f_pos - 1));
              }
              points.push_back({X, Y, Z, F});  
          }  
      }  
      gcode_file.close();  
  } else {  
      std::cout << "Nie można otworzyć pliku gcode.txt" << std::endl;  
  }
  gcode_file.close();
}