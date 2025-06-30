package main

import (
	"context"
	"flag"
	"fmt"
	"log"
	"math"

	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials/insecure"

	pb "magica/mcp/proto/magica_daw"
)

type UtilityAgent struct {
	client  pb.MagdaDAWServiceClient
	agentId string
	dawConn *grpc.ClientConn
}

func NewUtilityAgent(dawAddress string) (*UtilityAgent, error) {
	// Connect to Magda DAW
	conn, err := grpc.Dial(dawAddress, grpc.WithTransportCredentials(insecure.NewCredentials()))
	if err != nil {
		return nil, fmt.Errorf("failed to connect to DAW: %v", err)
	}

	client := pb.NewMagdaDAWServiceClient(conn)

	agent := &UtilityAgent{
		client:  client,
		dawConn: conn,
	}

	// Register with DAW
	if err := agent.register(); err != nil {
		conn.Close()
		return nil, fmt.Errorf("failed to register: %v", err)
	}

	return agent, nil
}

func (a *UtilityAgent) register() error {
	req := &pb.RegisterAgentRequest{
		Name: "UtilityAgent",
		Type: "utility",
		Capabilities: []string{
			"deduplicate_notes",
			"remove_short_notes",
			"quantize_notes",
			"normalize_velocity",
			"cleanup_recording",
		},
	}

	resp, err := a.client.RegisterAgent(context.Background(), req)
	if err != nil {
		return err
	}

	if !resp.Success {
		return fmt.Errorf("registration failed: %s", resp.Message)
	}

	a.agentId = resp.AgentId
	fmt.Printf("✓ Registered as agent: %s\n", a.agentId)
	return nil
}

func (a *UtilityAgent) DeduplicateClip(clipId string) error {
	fmt.Printf("🧹 Deduplicating notes in clip: %s\n", clipId)

	// 1. Get current notes
	getReq := &pb.GetMidiClipNotesRequest{ClipId: clipId}
	getResp, err := a.client.GetMidiClipNotes(context.Background(), getReq)
	if err != nil {
		return fmt.Errorf("failed to get notes: %v", err)
	}

	originalCount := len(getResp.Notes)
	fmt.Printf("   📝 Original notes: %d\n", originalCount)

	// 2. Deduplicate using map for O(1) lookup
	seen := make(map[string]bool)
	var uniqueNotes []*pb.MidiNote

	for _, note := range getResp.Notes {
		// Create unique key: pitch + start time (rounded to avoid float precision issues)
		key := fmt.Sprintf("%d_%.3f", note.Pitch, math.Round(note.StartTime*1000)/1000)

		if !seen[key] {
			seen[key] = true
			uniqueNotes = append(uniqueNotes, note)
		}
	}

	removedCount := originalCount - len(uniqueNotes)
	fmt.Printf("   🗑️  Removed duplicates: %d\n", removedCount)

	// 3. Update clip with deduplicated notes
	updateReq := &pb.UpdateMidiClipNotesRequest{
		ClipId: clipId,
		Notes:  uniqueNotes,
	}

	updateResp, err := a.client.UpdateMidiClipNotes(context.Background(), updateReq)
	if err != nil {
		return fmt.Errorf("failed to update notes: %v", err)
	}

	if !updateResp.Success {
		return fmt.Errorf("update failed")
	}

	fmt.Printf("   ✅ Deduplication complete: %d notes remaining\n", len(uniqueNotes))
	return nil
}

func (a *UtilityAgent) RemoveShortNotes(clipId string, minDuration float64) error {
	fmt.Printf("✂️  Removing notes shorter than %.3f beats in clip: %s\n", minDuration, clipId)

	// Get current notes
	getReq := &pb.GetMidiClipNotesRequest{ClipId: clipId}
	getResp, err := a.client.GetMidiClipNotes(context.Background(), getReq)
	if err != nil {
		return fmt.Errorf("failed to get notes: %v", err)
	}

	originalCount := len(getResp.Notes)

	// Filter out short notes
	var filteredNotes []*pb.MidiNote
	for _, note := range getResp.Notes {
		if note.Duration >= minDuration {
			filteredNotes = append(filteredNotes, note)
		}
	}

	removedCount := originalCount - len(filteredNotes)
	fmt.Printf("   🗑️  Removed short notes: %d\n", removedCount)

	// Update clip
	updateReq := &pb.UpdateMidiClipNotesRequest{
		ClipId: clipId,
		Notes:  filteredNotes,
	}

	updateResp, err := a.client.UpdateMidiClipNotes(context.Background(), updateReq)
	if err != nil {
		return fmt.Errorf("failed to update notes: %v", err)
	}

	if !updateResp.Success {
		return fmt.Errorf("update failed")
	}

	fmt.Printf("   ✅ Short note removal complete: %d notes remaining\n", len(filteredNotes))
	return nil
}

func (a *UtilityAgent) QuantizeClip(clipId string, gridSize float64) error {
	fmt.Printf("📐 Quantizing clip %s to %.3f beat grid\n", clipId, gridSize)

	quantizeReq := &pb.QuantizeClipRequest{
		ClipId:   clipId,
		GridSize: gridSize,
	}

	resp, err := a.client.QuantizeClip(context.Background(), quantizeReq)
	if err != nil {
		return fmt.Errorf("failed to quantize: %v", err)
	}

	if !resp.Success {
		return fmt.Errorf("quantization failed")
	}

	fmt.Printf("   ✅ Quantization complete\n")
	return nil
}

func (a *UtilityAgent) CleanupRecording(clipId string) error {
	fmt.Printf("🎯 Full cleanup process for clip: %s\n", clipId)

	// 1. Remove very short notes (likely accidents)
	if err := a.RemoveShortNotes(clipId, 0.05); err != nil {
		return fmt.Errorf("short note removal failed: %v", err)
	}

	// 2. Deduplicate
	if err := a.DeduplicateClip(clipId); err != nil {
		return fmt.Errorf("deduplication failed: %v", err)
	}

	// 3. Light quantization (16th note grid)
	if err := a.QuantizeClip(clipId, 0.25); err != nil {
		return fmt.Errorf("quantization failed: %v", err)
	}

	fmt.Printf("🎉 Recording cleanup complete for clip: %s\n", clipId)
	return nil
}

func (a *UtilityAgent) StartEventListener() {
	fmt.Printf("👂 Starting event listener...\n")

	// Subscribe to relevant events
	eventReq := &pb.AgentEventRequest{
		AgentId: a.agentId,
		EventTypes: []string{
			"CLIP_CREATED",
			"TRACK_CREATED",
			"RECORDING_STOPPED",
		},
	}

	stream, err := a.client.AgentEventStream(context.Background())
	if err != nil {
		log.Printf("Failed to start event stream: %v", err)
		return
	}

	// Send subscription request
	if err := stream.Send(eventReq); err != nil {
		log.Printf("Failed to subscribe to events: %v", err)
		return
	}

	// Listen for events
	go func() {
		for {
			event, err := stream.Recv()
			if err != nil {
				log.Printf("Event stream error: %v", err)
				return
			}

			a.handleEvent(event)
		}
	}()
}

func (a *UtilityAgent) handleEvent(event *pb.AgentEventResponse) {
	fmt.Printf("📨 Received event: %s\n", event.EventType)

	switch event.EventType {
	case "RECORDING_STOPPED":
		// Auto-cleanup recordings
		fmt.Printf("🎤 Recording stopped, offering cleanup...\n")
		// In a real implementation, you'd parse event.EventData to get clip ID
		// and potentially offer automatic cleanup

	case "CLIP_CREATED":
		fmt.Printf("📝 New clip created\n")

	case "TRACK_CREATED":
		fmt.Printf("🎵 New track created\n")
	}
}

func (a *UtilityAgent) Close() {
	if a.dawConn != nil {
		a.dawConn.Close()
	}
}

func main() {
	dawAddress := flag.String("daw", "localhost:50051", "Magda DAW server address")
	clipId := flag.String("clip", "", "Clip ID to process (optional)")
	action := flag.String("action", "listen", "Action: listen, cleanup, dedupe, quantize")
	flag.Parse()

	fmt.Printf("🤖 Starting Utility Agent...\n")

	// Connect to DAW
	agent, err := NewUtilityAgent(*dawAddress)
	if err != nil {
		log.Fatalf("Failed to create agent: %v", err)
	}
	defer agent.Close()

	fmt.Printf("🔗 Connected to Magda DAW at %s\n", *dawAddress)

	// Handle different actions
	switch *action {
	case "listen":
		fmt.Printf("👂 Listening for events... (Press Ctrl+C to exit)\n")
		agent.StartEventListener()

		// Keep running
		select {}

	case "cleanup":
		if *clipId == "" {
			log.Fatal("clip ID required for cleanup action")
		}
		if err := agent.CleanupRecording(*clipId); err != nil {
			log.Fatalf("Cleanup failed: %v", err)
		}

	case "dedupe":
		if *clipId == "" {
			log.Fatal("clip ID required for dedupe action")
		}
		if err := agent.DeduplicateClip(*clipId); err != nil {
			log.Fatalf("Deduplication failed: %v", err)
		}

	case "quantize":
		if *clipId == "" {
			log.Fatal("clip ID required for quantize action")
		}
		if err := agent.QuantizeClip(*clipId, 0.25); err != nil {
			log.Fatalf("Quantization failed: %v", err)
		}

	default:
		log.Fatalf("Unknown action: %s", *action)
	}

	fmt.Printf("✅ Agent task completed\n")
}
