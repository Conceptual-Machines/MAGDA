#!/usr/bin/env python3

import argparse
import asyncio
import json
import logging
import openai
from typing import Dict, List, Optional

import grpc
from mcp.proto import magica_daw_pb2 as pb
from mcp.proto import magica_daw_pb2_grpc as pb_grpc

# Set up logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

class OrchestratorAgent:
    def __init__(self, daw_address: str, openai_api_key: Optional[str] = None):
        self.daw_address = daw_address
        self.agent_id = None
        self.channel = None
        self.client = None
        self.connected_agents = {}
        
        # Initialize OpenAI if API key provided
        if openai_api_key:
            openai.api_key = openai_api_key
            self.llm_available = True
            logger.info("🧠 OpenAI LLM enabled")
        else:
            self.llm_available = False
            logger.info("🧠 Running without LLM (pattern matching only)")
    
    async def connect(self):
        """Connect to Magda DAW and register as orchestrator agent"""
        logger.info(f"🔗 Connecting to Magda DAW at {self.daw_address}")
        
        self.channel = grpc.aio.insecure_channel(self.daw_address)
        self.client = pb_grpc.MagdaDAWServiceStub(self.channel)
        
        # Register as orchestrator agent
        register_req = pb.RegisterAgentRequest(
            name="OrchestratorAgent",
            type="orchestrator",
            capabilities=[
                "natural_language_processing",
                "agent_coordination", 
                "workflow_management",
                "task_routing",
                "session_analysis"
            ]
        )
        
        try:
            register_resp = await self.client.RegisterAgent(register_req)
            if register_resp.success:
                self.agent_id = register_resp.agent_id
                logger.info(f"✅ Registered as orchestrator: {self.agent_id}")
                return True
            else:
                logger.error(f"❌ Registration failed: {register_resp.message}")
                return False
        except grpc.RpcError as e:
            logger.error(f"❌ Connection failed: {e}")
            return False
    
    async def get_connected_agents(self):
        """Get list of all connected agents"""
        try:
            resp = await self.client.GetConnectedAgents(pb.GetConnectedAgentsRequest())
            self.connected_agents = {agent.agent_id: agent for agent in resp.agents}
            logger.info(f"🤖 Found {len(resp.agents)} connected agents:")
            for agent in resp.agents:
                logger.info(f"   • {agent.name} ({agent.type}): {', '.join(agent.capabilities)}")
            return resp.agents
        except grpc.RpcError as e:
            logger.error(f"❌ Failed to get agents: {e}")
            return []
    
    async def analyze_user_intent(self, user_prompt: str, session_context: Dict) -> Dict:
        """Analyze user intent and determine required actions"""
        
        if self.llm_available:
            return await self._analyze_with_llm(user_prompt, session_context)
        else:
            return await self._analyze_with_patterns(user_prompt, session_context)
    
    async def _analyze_with_llm(self, user_prompt: str, session_context: Dict) -> Dict:
        """Use LLM to analyze user intent"""
        system_prompt = f"""
        You are an AI music production orchestrator. Analyze user requests and determine:
        1. What agents are needed
        2. What sequence of actions to perform
        3. What parameters to use
        
        Available agents: {list(self.connected_agents.keys())}
        Agent capabilities: {[(a.name, a.capabilities) for a in self.connected_agents.values()]}
        
        Current session: {json.dumps(session_context, indent=2)}
        
        User request: "{user_prompt}"
        
        Respond with JSON:
        {{
            "intent": "description of what user wants",
            "complexity": "simple|medium|complex",
            "workflow": [
                {{
                    "agent_type": "utility|melody|percussion|etc",
                    "action": "specific action to perform",
                    "parameters": {{"param1": "value1"}},
                    "depends_on": ["previous_step_ids"]
                }}
            ]
        }}
        """
        
        try:
            response = await openai.ChatCompletion.acreate(
                model="gpt-4",
                messages=[
                    {"role": "system", "content": system_prompt},
                    {"role": "user", "content": user_prompt}
                ],
                max_tokens=1000,
                temperature=0.3
            )
            
            analysis = json.loads(response.choices[0].message.content)
            logger.info(f"🧠 LLM Analysis: {analysis['intent']}")
            return analysis
            
        except Exception as e:
            logger.error(f"❌ LLM analysis failed: {e}")
            return await self._analyze_with_patterns(user_prompt, session_context)
    
    async def _analyze_with_patterns(self, user_prompt: str, session_context: Dict) -> Dict:
        """Fallback pattern-based intent analysis"""
        prompt_lower = user_prompt.lower()
        
        # Pattern matching for common requests
        if any(word in prompt_lower for word in ["clean", "cleanup", "fix", "duplicate", "short"]):
            return {
                "intent": "Clean up MIDI recording",
                "complexity": "simple",
                "workflow": [{
                    "agent_type": "utility",
                    "action": "cleanup_recording",
                    "parameters": {"clip_id": session_context.get("selected_clip")},
                    "depends_on": []
                }]
            }
        
        elif any(word in prompt_lower for word in ["melody", "tune", "song", "compose"]):
            return {
                "intent": "Generate melody",
                "complexity": "medium", 
                "workflow": [{
                    "agent_type": "melody",
                    "action": "generate_melody",
                    "parameters": {
                        "style": "detect from prompt",
                        "key": session_context.get("key", "C major"),
                        "length_bars": 8
                    },
                    "depends_on": []
                }]
            }
        
        elif any(word in prompt_lower for word in ["drum", "beat", "rhythm", "percussion"]):
            return {
                "intent": "Generate drum pattern",
                "complexity": "medium",
                "workflow": [{
                    "agent_type": "percussion", 
                    "action": "generate_drums",
                    "parameters": {
                        "style": "detect from prompt",
                        "tempo": session_context.get("tempo", 120),
                        "bars": 4
                    },
                    "depends_on": []
                }]
            }
        
        else:
            return {
                "intent": "Unknown request",
                "complexity": "complex",
                "workflow": []
            }
    
    async def execute_workflow(self, workflow: List[Dict]):
        """Execute the analyzed workflow"""
        logger.info(f"🎯 Executing workflow with {len(workflow)} steps")
        
        completed_steps = {}
        
        for i, step in enumerate(workflow):
            step_id = f"step_{i}"
            logger.info(f"📋 Step {i+1}: {step['action']} via {step['agent_type']} agent")
            
            # Check dependencies
            if step.get("depends_on"):
                for dep in step["depends_on"]:
                    if dep not in completed_steps:
                        logger.error(f"❌ Dependency {dep} not completed")
                        continue
            
            # Find appropriate agent
            target_agent = None
            for agent in self.connected_agents.values():
                if agent.type == step["agent_type"]:
                    target_agent = agent
                    break
            
            if not target_agent:
                logger.error(f"❌ No {step['agent_type']} agent available")
                continue
            
            # Send task to agent
            message = {
                "task": step["action"],
                "parameters": step["parameters"],
                "step_id": step_id
            }
            
            send_req = pb.SendMessageRequest(
                target_agent_id=target_agent.agent_id,
                message=json.dumps(message)
            )
            
            try:
                resp = await self.client.SendMessageToAgent(send_req)
                if resp.success:
                    logger.info(f"✅ Step {i+1} sent to {target_agent.name}")
                    completed_steps[step_id] = True
                else:
                    logger.error(f"❌ Step {i+1} failed: {resp.message}")
            except grpc.RpcError as e:
                logger.error(f"❌ Step {i+1} error: {e}")
    
    async def get_session_context(self) -> Dict:
        """Get current DAW session context"""
        try:
            # Get transport state
            transport_resp = await self.client.GetTransportState(pb.GetTransportStateRequest())
            transport = transport_resp.state
            
            # Get tracks
            tracks_resp = await self.client.GetTracks(pb.GetTracksRequest())
            tracks = tracks_resp.tracks
            
            # Get session info
            session_resp = await self.client.GetSessionInfo(pb.GetSessionInfoRequest())
            session = session_resp
            
            context = {
                "tempo": transport.tempo,
                "time_signature": f"{transport.time_sig_num}/{transport.time_sig_den}",
                "current_time": transport.current_time,
                "playing": transport.playing,
                "track_count": len(tracks),
                "tracks": [{"id": t.track_id, "name": t.name, "type": t.type} for t in tracks],
                "session_name": session.session_name,
                "sample_rate": session.sample_rate
            }
            
            return context
            
        except grpc.RpcError as e:
            logger.error(f"❌ Failed to get session context: {e}")
            return {}
    
    async def process_user_request(self, user_prompt: str):
        """Main function to process user natural language request"""
        logger.info(f"🎤 Processing: '{user_prompt}'")
        
        # 1. Get current session context
        context = await self.get_session_context()
        logger.info(f"📊 Session context: {context.get('track_count', 0)} tracks, {context.get('tempo', 'unknown')} BPM")
        
        # 2. Get available agents
        await self.get_connected_agents()
        
        # 3. Analyze user intent
        analysis = await self.analyze_user_intent(user_prompt, context)
        logger.info(f"🎯 Intent: {analysis['intent']} (complexity: {analysis['complexity']})")
        
        # 4. Execute workflow
        if analysis.get("workflow"):
            await self.execute_workflow(analysis["workflow"])
        else:
            logger.warning("⚠️  No workflow generated - request not understood")
    
    async def start_interactive_mode(self):
        """Start interactive mode for user input"""
        logger.info("🎹 Interactive mode started. Type 'quit' to exit.")
        logger.info("Examples:")
        logger.info("  • 'Clean up the piano recording'")
        logger.info("  • 'Add a jazz melody in C major'") 
        logger.info("  • 'Generate a rock drum pattern'")
        print()
        
        while True:
            try:
                user_input = input("🎵 Magda> ").strip()
                
                if user_input.lower() in ['quit', 'exit', 'q']:
                    logger.info("👋 Goodbye!")
                    break
                
                if user_input:
                    await self.process_user_request(user_input)
                    print()
                    
            except KeyboardInterrupt:
                logger.info("\n👋 Goodbye!")
                break
            except Exception as e:
                logger.error(f"❌ Error: {e}")
    
    async def close(self):
        """Clean up connection"""
        if self.channel:
            await self.channel.close()

async def main():
    parser = argparse.ArgumentParser(description="Magda DAW Orchestrator Agent")
    parser.add_argument("--daw", default="localhost:50051", 
                       help="Magda DAW server address")
    parser.add_argument("--openai-key", 
                       help="OpenAI API key for LLM analysis")
    parser.add_argument("--request", 
                       help="Single request to process (non-interactive)")
    
    args = parser.parse_args()
    
    # Create orchestrator
    orchestrator = OrchestratorAgent(args.daw, args.openai_key)
    
    try:
        # Connect to DAW
        if not await orchestrator.connect():
            return 1
        
        if args.request:
            # Process single request
            await orchestrator.process_user_request(args.request)
        else:
            # Interactive mode
            await orchestrator.start_interactive_mode()
        
    except Exception as e:
        logger.error(f"❌ Fatal error: {e}")
        return 1
    finally:
        await orchestrator.close()
    
    return 0

if __name__ == "__main__":
    exit(asyncio.run(main())) 