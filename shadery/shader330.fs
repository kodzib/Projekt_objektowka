#version 330

// Input vertex attributes (from vertex shader)
in vec3 fragPosition;
in vec2 fragTexCoord;
in vec3 fragNormal;
in vec4 fragPosLightSpace; // Light-space position passed from vertex shader

// Output fragment color
out vec4 finalColor;

// Uniforms
uniform sampler2D texture0;
uniform vec4 colDiffuse;

uniform vec3 lightDir;
uniform vec4 lightColor;
uniform vec4 ambient;
uniform vec3 viewPos;

uniform sampler2D shadowMap;
uniform int shadowMapResolution;

void main()
{
    vec4 texelColor = texture(texture0, fragTexCoord);
    vec3 normal = normalize(fragNormal);
    vec3 viewD = normalize(viewPos - fragPosition);
    vec3 lightVec = -lightDir;

    // Basic lighting
    float NdotL = max(dot(normal, lightVec), 0.0);
    vec3 diffuse = lightColor.rgb * NdotL;

    float specCo = (NdotL > 0.0) ? pow(max(dot(viewD, reflect(-lightVec, normal)), 0.0), 16.0) : 0.0;
    vec3 specular = vec3(specCo);

    vec4 litColor = texelColor * ((colDiffuse + vec4(specular, 1.0)) * vec4(diffuse, 1.0));

    // Transform light-space position
    vec4 projCoords = fragPosLightSpace;
    projCoords.xyz /= projCoords.w;
    projCoords.xyz = projCoords.xyz * 0.5 + 0.5;

    vec2 sampleCoords = projCoords.xy;
    float currentDepth = projCoords.z;

    // Bias to prevent shadow acne
    float bias = max(0.0004 * (1.0 - dot(normal, lightVec)), 0.00002) + 0.00001;

    // PCF: 5x5 filter for soft shadows
    const int kernelSize = 2;
    const int numSamples = (2 * kernelSize + 1) * (2 * kernelSize + 1);
    vec2 texelSize = vec2(1.0 / float(shadowMapResolution));

    int shadowCount = 0;
    for (int x = -kernelSize; x <= kernelSize; x++)
    {
        for (int y = -kernelSize; y <= kernelSize; y++)
        {
            vec2 offset = texelSize * vec2(float(x), float(y));
            vec2 coord = clamp(sampleCoords + offset, 0.0, 1.0);
            float sampleDepth = texture(shadowMap, coord).r;
            if (currentDepth - bias > sampleDepth)
                shadowCount++;
        }
    }

    float shadowFactor = float(shadowCount) / float(numSamples);
    shadowFactor = smoothstep(0.0, 1.0, shadowFactor); // optional smoothing

    // Shadow blending using brightness factor
    float brightness = mix(1.0, 0.3, shadowFactor); // Less harsh than full black
    litColor.rgb *= brightness;

    // Add ambient even in shadow
    vec3 ambientLight = texelColor.rgb * (ambient.rgb * colDiffuse.rgb);
    litColor.rgb = mix(litColor.rgb, ambientLight, shadowFactor * 0.5);

    // Gamma correction
    finalColor = pow(litColor, vec4(1.0 / 2.2));
}
