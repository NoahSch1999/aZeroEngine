struct BoundingSphere
{
    float3 Position;
    float Radius;
};

// Why doesn't hlsl support class constructors...
// Maybe look into doing this: https://colinbarrebrisebois.com/2021/11/01/working-around-constructors-in-hlsl-or-lack-thereof/
BoundingSphere CreateBoundingSphere(float3 Position, float Radius) 
{
    BoundingSphere sphere;
    sphere.Position = Position;
    sphere.Radius = Radius;
    return sphere;
}

struct Frustum
{
    float3 Position;
    float4 Rotation;
    float RightSlope;
    float LeftSlope;
    float TopSlope;
    float BottomSlope;
    float Near;
    float Far;
    
    bool Intersects(BoundingSphere sphere, float4x4 viewMatrix)
    {
        float4 planes[6];
        planes[0] = float4(0.0f, 0.0f, -1.0f, Near);
        planes[1] = float4(0.0f, 0.0f, 1.0f, -Far);
        planes[2] = float4(1.0f, 0.0f, -RightSlope, 0.0f);
        planes[3] = float4(-1.0f, 0.0f, LeftSlope, 0.0f);
        planes[4] = float4(0.0f, 1.0f, -TopSlope, 0.0f);
        planes[5] = float4(0.0f, -1.0f, BottomSlope, 0.0f);
        
        planes[2] = normalize(planes[2]);
        planes[3] = normalize(planes[3]);
        planes[4] = normalize(planes[4]);
        planes[5] = normalize(planes[5]);
        
        float4 sphereLocalPos = mul(viewMatrix, float4(sphere.Position - Position, 1.f));
        sphereLocalPos.w = 1.f;
        
        float dist[6];
        for (int i = 0; i < 6; i++)
        {
            dist[i] = dot(sphereLocalPos, planes[i]);
            bool isOutside = dist[i] > sphere.Radius;
            if(isOutside)
            {
                return false;
            }
        }
        
        return true;
    }
};

Frustum CreateFrustum(float3 position, float4 rotation, float rightSlope, float leftSlope, float topSlope, float bottomSlope, float near, float far)
{
    Frustum frustum;
    frustum.Position = position;
    frustum.Rotation = rotation;
    frustum.RightSlope = rightSlope;
    frustum.LeftSlope = leftSlope;
    frustum.TopSlope = topSlope;
    frustum.BottomSlope = bottomSlope;
    frustum.Near = near;
    frustum.Far = far;
    return frustum;
}

Frustum CreateFrustumFromCameraMatrix(float4x4 viewProjectionMatrix)
{
    // TODO: Calc
    float3 position;
    float4 rotation;
    float rightSlope;
    float leftSlope;
    float topSlope;
    float bottomSlope;
    float near;
    float far;
    return CreateFrustum(position, rotation, rightSlope, leftSlope, topSlope, bottomSlope, near, far);
}